import { atom } from 'recoil';

export enum MessageType {
  SinglePress = 'single_press',
}

export interface IDataElement<T> {
  value: T;
  time: Date;
}

export const connectedState = atom<boolean>({
  key: 'connected',
  default: false,
});

export const sendMessageState = atom<IDataElement<MessageType> | null>({
  key: 'send-message-data',
  default: null,
});

export const voltageState = atom<IDataElement<number> | null>({
  key: 'voltage-data',
  default: null,
});
