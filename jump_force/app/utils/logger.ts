import { logger as log } from 'react-native-logs';
import { LogBox } from 'react-native';

LogBox.ignoreLogs(['Setting a timer for a long period of time']);

const logger = log.createLogger();

export default logger;
