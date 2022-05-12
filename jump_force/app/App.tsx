import React, { FunctionComponent } from 'react';
import { StatusBar } from 'expo-status-bar';

import { Box, NativeBaseProvider } from 'native-base';
import { RecoilRoot } from 'recoil';
import { NavigationContainer } from '@react-navigation/native';
import Navigation from './components/Navigation';

const App: FunctionComponent = function () {
  return (
    <NavigationContainer>
      <NativeBaseProvider>
        <RecoilRoot>
          <Box safeArea flex={1}>
            <StatusBar backgroundColor="grey" translucent={false} />
            <Navigation />
          </Box>
        </RecoilRoot>
      </NativeBaseProvider>
    </NavigationContainer>
  );
};

export default App;
