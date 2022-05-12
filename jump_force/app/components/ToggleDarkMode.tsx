import React, { FunctionComponent } from 'react';

import { Text, HStack, Switch, useColorMode } from 'native-base';

const ToggleDarkMode: FunctionComponent = function () {
  const { colorMode, toggleColorMode } = useColorMode();
  return (
    <HStack px="5" space={2} alignItems="center">
      <Text>Dark</Text>
      <Switch
        isChecked={colorMode === 'light'}
        onToggle={toggleColorMode}
        aria-label={
          colorMode === 'light' ? 'switch to dark mode' : 'switch to light mode'
        }
      />
      <Text>Light</Text>
    </HStack>
  );
};

export default ToggleDarkMode;
