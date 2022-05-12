import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

const App = function () {
  return (
    <View style={styles.container}>
      <Text style={styles.text}>Welcome to Jump Force Web</Text>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  text: {
    fontSize: 16,
  },
});

export default App;
