import React from 'react';
import { Text, View } from 'native-base';
import { TouchableOpacity } from 'react-native';
import { DrawerNavigationProp } from '@react-navigation/drawer';
import { RootParamList } from './types';

const NotFoundScreen = function (
  args: DrawerNavigationProp<RootParamList, 'NotFound'>,
) {
  return (
    <View>
      <Text>This screen doesn`&apos;`t exist.</Text>
      <TouchableOpacity onPress={() => args.navigate('Home')}>
        <Text>Go to home screen!</Text>
      </TouchableOpacity>
    </View>
  );
};

export default NotFoundScreen;
