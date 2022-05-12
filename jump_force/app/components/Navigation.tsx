import React, { useCallback } from 'react';
import {
  Box,
  Divider,
  Heading,
  HStack,
  Icon,
  Pressable,
  Text,
  VStack,
} from 'native-base';
import {
  createDrawerNavigator,
  DrawerContentComponentProps,
  DrawerContentScrollView,
} from '@react-navigation/drawer';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import HomeScreen from '../screens/HomeScreen';
import { RootParamList } from '../screens/types';
import AboutScreen from '../screens/AboutScreen';
import ToggleDarkMode from './ToggleDarkMode';

const Drawer = createDrawerNavigator<RootParamList>();

const DrawerContent = function (props: DrawerContentComponentProps) {
  const getIcon = useCallback((name: string): string => {
    switch (name) {
      case 'Home':
        return 'home';
      case 'About':
        return 'information';
    }
    return '';
  }, []);

  return (
    <DrawerContentScrollView {...props}>
      <VStack space="6" my="2" mx="1">
        <Box px="4">
          <Heading>Jump Force</Heading>
        </Box>
        <VStack space="3">
          {props.state.routeNames.map((name, index) => (
            <Pressable
              key={`nav-button-${name}`}
              px="5"
              py="3"
              rounded="md"
              bg={
                index === props.state.index
                  ? 'rgba(6, 182, 212, 0.1)'
                  : 'transparent'
              }
              onPress={() => {
                props.navigation.navigate(name);
              }}
            >
              <HStack space="7" alignItems="center">
                <Icon
                  color={
                    index === props.state.index ? 'primary.500' : 'gray.500'
                  }
                  size="5"
                  as={<MaterialCommunityIcons name={getIcon(name) as any} />}
                />
                <Text
                  fontWeight="500"
                  color={
                    index === props.state.index ? 'primary.500' : 'gray.700'
                  }
                >
                  {name}
                </Text>
              </HStack>
            </Pressable>
          ))}
        </VStack>
        <Divider />
        <ToggleDarkMode />
      </VStack>
    </DrawerContentScrollView>
  );
};

const Navigation = function () {
  return (
    <Box safeArea flex={1}>
      {/* eslint-disable-next-line react/no-unstable-nested-components */}
      <Drawer.Navigator drawerContent={(props) => <DrawerContent {...props} />}>
        <Drawer.Screen name="Home" component={HomeScreen} />
        <Drawer.Screen name="About" component={AboutScreen} />
      </Drawer.Navigator>
    </Box>
  );
};

export default Navigation;
