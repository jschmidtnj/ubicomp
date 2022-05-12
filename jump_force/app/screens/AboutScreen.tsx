import React from 'react';
import { Center, Heading, Text } from 'native-base';
import { DrawerNavigationProp } from '@react-navigation/drawer';
import { RootParamList } from './types';
import Layout from './Layout';

const AboutScreen = function (
  _args: DrawerNavigationProp<RootParamList, 'About'>,
) {
  return (
    <Layout>
      <Center pt="8" pb="20" px={4}>
        <Heading size="lg">About</Heading>
        <Text>This is Jump Force</Text>
      </Center>
    </Layout>
  );
};

export default AboutScreen;
