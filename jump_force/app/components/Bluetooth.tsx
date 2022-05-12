import React, {
  FunctionComponent,
  useCallback,
  useEffect,
  useMemo,
  useState,
} from 'react';
import { Button, Text, useToast, View } from 'native-base';
import { PermissionsAndroid, Platform } from 'react-native';
import {
  BleError,
  BleManager,
  Characteristic,
  Device,
  Subscription,
} from 'react-native-ble-plx';
import base64 from 'react-native-base64';
import { useRecoilState } from 'recoil';
import logger from '../utils/logger';
import {
  connectedState,
  sendMessageState,
  MessageType,
  voltageState,
} from '../state/data';
import sleep from '../utils/sleep';

const Bluetooth: FunctionComponent = function () {
  const toast = useToast();

  const [manager, setManager] = useState<BleManager | undefined>(undefined);
  const [deviceObj, setDeviceObj] = useState<Device | undefined>(undefined);

  const [connected, setConnected] = useRecoilState(connectedState);

  const [sendMessageData, setSendMessageData] =
    useRecoilState(sendMessageState);

  const handleSendMessage = useCallback(
    async (
      error: BleError | null,
      givenCharacteristic: Characteristic | null,
    ): Promise<void> => {
      try {
        if (error) {
          throw error;
        }
        if (!givenCharacteristic) {
          throw new Error('no data');
        }
        const characteristic = await givenCharacteristic.read();
        const currDataStr = base64.decode(characteristic.value as string);
        if (!Object.values(MessageType).includes(currDataStr as MessageType)) {
          throw new Error(`invalid message found: ${currDataStr}`);
        }
        const now = new Date();
        setSendMessageData({
          time: now,
          value: currDataStr as MessageType,
        });
      } catch (err) {
        const errObj = err as Error;
        logger.error(errObj.message);
      }
    },
    [setSendMessageData],
  );

  const [voltageData, setVoltageData] = useRecoilState(voltageState);

  const handleVoltage = useCallback(
    async (
      error: BleError | null,
      givenCharacteristic: Characteristic | null,
    ): Promise<void> => {
      try {
        if (error) {
          throw error;
        }
        if (!givenCharacteristic) {
          throw new Error('no data');
        }
        const characteristic = await givenCharacteristic.read();
        const currDataStr = base64.decode(characteristic.value as string);
        const currData = Number(currDataStr);
        if (!currData) {
          throw new Error(`cannot cast ${currDataStr} to number`);
        }
        const now = new Date();
        setVoltageData({
          time: now,
          value: currData,
        });
      } catch (err) {
        const errObj = err as Error;
        logger.error(errObj.message);
      }
    },
    [setVoltageData],
  );

  const uuids = useMemo(
    () => [
      process.env.MESSAGE_SEND_CHARACTERISTIC_UUID,
      process.env.VOLTAGE_CHARACTERISTIC_UUID,
    ],
    [],
  );

  const uuidHandlers = useMemo(() => {
    if (uuids.some((elem) => elem === undefined)) {
      return {};
    }
    return {
      [uuids[0] as string]: handleSendMessage,
      [uuids[1] as string]: handleVoltage,
    };
  }, [handleSendMessage, handleVoltage, uuids]);

  const [subscriptions, setSubscriptions] = useState<Subscription[]>([]);

  const connectDevice = useCallback(
    async (givenDevice: Device) => {
      let device = await givenDevice.connect();
      device = await device.discoverAllServicesAndCharacteristics();
      const services = await device.services();

      const getCharacteristics: Promise<Characteristic[]>[] = [];
      for (const service of services) {
        if (!service.isPrimary) continue;
        getCharacteristics.push(service.characteristics());
      }

      const subscribeCharacteristics = (
        await Promise.all(getCharacteristics)
      ).flatMap((characteristics) =>
        characteristics.filter((characteristic) =>
          uuids.includes(characteristic.uuid),
        ),
      );
      const missingCharacteristicUUIDs = uuids.filter(
        (key) => !subscribeCharacteristics.some((elem) => elem.uuid === key),
      );
      if (missingCharacteristicUUIDs.length > 0) {
        throw new Error(
          `missing characteristics with ids ${missingCharacteristicUUIDs}`,
        );
      }
      setSubscriptions(
        subscribeCharacteristics.map((characteristic) =>
          characteristic.monitor(
            uuidHandlers[characteristic.uuid as keyof typeof uuidHandlers],
          ),
        ),
      );

      logger.info('start monitoring');
      setConnected(true);
      setDeviceObj(device);
    },
    [setConnected, uuidHandlers, uuids],
  );

  const scanAndConnect = useCallback(() => {
    if (!manager) return;

    if (!process.env.SERVICE_UUID) {
      throw new Error('cannot find service uuid');
    }
    const missingUUID = uuids.find((elem) => elem === undefined);
    if (missingUUID) {
      throw new Error(`cannot find uuid ${missingUUID}`);
    }

    logger.info('start scanning');
    manager.startDeviceScan(null, null, async (error, givenDevice) => {
      try {
        if (error) {
          throw error;
        }
        if (!givenDevice) {
          throw new Error('cannot find device');
        }

        if (givenDevice.serviceUUIDs?.includes(process.env.SERVICE_UUID!)) {
          manager.stopDeviceScan();
          await connectDevice(givenDevice);
        }
      } catch (err) {
        const errObj = err as Error;
        toast.show({
          title: 'Scan Error',
          description: errObj.message,
          bgColor: 'red.700',
          duration: 1000,
        });
      }
    });
  }, [manager, uuids, connectDevice, toast]);

  const disconnect = useCallback(async (): Promise<void> => {
    logger.info('start disconnect');

    try {
      subscriptions.map((subscription) => subscription.remove());
      if (manager !== undefined && deviceObj !== undefined) {
        await manager.cancelDeviceConnection(deviceObj.id);
      }
    } catch (err) {
      const errObj = err as Error;
      toast.show({
        title: 'Disconnect Error',
        description: errObj.message,
        bgColor: 'red.700',
        duration: 1000,
      });
    } finally {
      setSubscriptions([]);
      setDeviceObj(undefined);
      setConnected(false);

      // reset all data
      setSendMessageData(null);
    }
  }, [
    subscriptions,
    manager,
    deviceObj,
    toast,
    setConnected,
    setSendMessageData,
  ]);

  const connect = useCallback(async () => {
    if (!manager) return;
    logger.info('start connect');

    try {
      await disconnect();
      await sleep(1000);
      let bluetoothEnabled = false;
      if (Platform.OS === 'android' && Platform.Version >= 23) {
        bluetoothEnabled = await PermissionsAndroid.check(
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        );
        if (!bluetoothEnabled) {
          bluetoothEnabled =
            (await PermissionsAndroid.request(
              PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
            )) === 'granted';
        }
      } else {
        bluetoothEnabled = true;
      }
      if (!bluetoothEnabled) {
        throw new Error('cannot setup bluetooth connection');
      }

      const currSub = manager.onStateChange((state) => {
        try {
          if (state === 'PoweredOn') {
            scanAndConnect();
            currSub.remove();
          }
        } catch (err) {
          const errObj = err as Error;
          toast.show({
            title: 'scan and connect error',
            description: errObj.message,
            bgColor: 'red.700',
            duration: 1000,
          });
        }
      }, true);
    } catch (err) {
      const errObj = err as Error;
      toast.show({
        title: 'Connect Error',
        description: errObj.message,
        bgColor: 'red.700',
        duration: 1000,
      });
    }
  }, [manager, disconnect, scanAndConnect, toast]);

  useEffect(() => {
    setManager(
      new BleManager({
        restoreStateIdentifier: 'jump-force-ble-manager-restore-state',
        restoreStateFunction: async (bleRestoredState) => {
          if (
            !bleRestoredState ||
            bleRestoredState.connectedPeripherals.length === 0
          ) {
            // BleManager was constructed for the first time.
            return;
          }
          // BleManager was restored
          try {
            await connectDevice(bleRestoredState.connectedPeripherals[0]);
          } catch (err) {
            const errObj = err as Error;
            logger.error(errObj.message);
          }
        },
      }),
    );
    logger.debug('added ble manager');
  }, [connectDevice]);

  return (
    <View>
      <Text>
        {connected && deviceObj ? `connected to ${deviceObj.name}` : ''}
      </Text>
      {!deviceObj ? (
        <Button onPress={connect}>Connect</Button>
      ) : (
        <View>
          <Button onPress={disconnect}>Disconnect</Button>
          {!sendMessageData ? null : (
            <Text>{`send message: ${sendMessageData.value}`}</Text>
          )}
          {!voltageData ? null : <Text>{`voltage: ${voltageData.value}`}</Text>}
        </View>
      )}
    </View>
  );
};

export default Bluetooth;
