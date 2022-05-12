import { ConfigContext, ExpoConfig } from '@expo/config';

const version = '1.0.0';
// increment by 1 on each build
const versionCode = 1;

export default ({ config }: ConfigContext): ExpoConfig => ({
  ...config,
  name: 'Jump Force',
  slug: 'jump_force',
  version,
  owner: 'jump-force',
  orientation: 'portrait',
  icon: './assets/images/icon.png',
  scheme: 'jumpforce',
  userInterfaceStyle: 'automatic',
  splash: {
    image: './assets/images/logo.png',
    resizeMode: 'contain',
    backgroundColor: '#18181B',
  },
  extra: {
    eas: {
      projectId: '90088f5a-6f49-47fe-9484-1bc5d6ec7cda',
    },
  },
  updates: {
    fallbackToCacheTimeout: 0,
  },
  assetBundlePatterns: ['**/*'],
  ios: {
    supportsTablet: true,
    // needs to be the same as the current ios version
    bundleIdentifier: 'com.jumpforce.jumpforce',
    // see https://docs.expo.dev/versions/latest/sdk/screen-orientation/#warning
    requireFullScreen: true,
  },
  android: {
    permissions: [
      'android.permission.BLUETOOTH',
      'android.permission.BLUETOOTH_ADMIN',
    ],
    package: 'com.jumpforce.jumpforce',
    versionCode,
  },
  web: {
    favicon: './assets/images/favicon.png',
  },
  backgroundColor: '#18181B',
});
