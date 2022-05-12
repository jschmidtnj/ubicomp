import { ScrollView } from 'native-base';
import React, { FunctionComponent } from 'react';

const Layout: FunctionComponent = function (args) {
  return (
    <ScrollView _dark={{ bg: 'blueGray.900' }} _light={{ bg: 'blueGray.50' }}>
      {args.children}
    </ScrollView>
  );
};

export default Layout;
