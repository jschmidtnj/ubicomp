import React from 'react';
import { Button, Center, Heading, VStack } from 'native-base';
import { DrawerNavigationProp } from '@react-navigation/drawer';
import Bluetooth from '../components/Bluetooth';
import { RootParamList } from './types';
import Layout from './Layout';

const HomeScreen = function (
  _args: DrawerNavigationProp<RootParamList, 'Home'>,
) {
  return (
    <Layout>
      <Center pt="8" pb="20" px={4}>
        <VStack space={5} alignItems="center">
          <Heading size="lg">Welcome to Jump Force 🚀</Heading>
          <Bluetooth />
          <Button mt="4">Start Test</Button>
        </VStack>
      </Center>
    </Layout>
  );
};

export default HomeScreen;
