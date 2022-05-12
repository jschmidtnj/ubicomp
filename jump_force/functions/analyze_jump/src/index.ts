import {
  AdminAddUserToGroupCommand,
  AdminUpdateUserAttributesCommand
} from '@aws-sdk/client-cognito-identity-provider';
import { web3 } from '@project-serum/anchor';
import type { PostConfirmationTriggerHandler } from 'aws-lambda';
import {
  api,
  appSyncClient,
  cognitoUserPoolID,
  idpClient,
  initializeConfigFunction,
  userIDClaim
} from 'functions';
import gql from 'graphql-tag';
import { getLogger } from 'log4js';

const logger = getLogger();

interface IUserAttributes {
  email: string;
  name: string;
}

const userExists = async (data: IUserAttributes): Promise<boolean> => {
  const vars: api.ListUsersWithEmailQueryVariables = {
    email: data.email
  };

  const res = await appSyncClient.query<api.ListUsersWithEmailQuery>({
    query: gql(api.ListUsersWithEmail),
    variables: vars
  });
  if (!res.data || !res.data.listUsers) {
    throw new Error('no data found in response');
  }

  return res.data.listUsers.items.length > 0;
};

const userCreate = async (
  data: IUserAttributes,
  username: string
): Promise<string> => {
  const solanaKey = web3.Keypair.generate();
  const textDecoder = new TextDecoder();
  const vars: api.CreateUserMutationVariables = {
    input: {
      email: data.email,
      name: data.name,
      solanaPublicKey: solanaKey.publicKey.toString(),
      solanaPrivateKey: textDecoder.decode(solanaKey.secretKey),
      savedData: [],
      type: api.UserType.USER,
      username,
      showAllLinks: false
    }
  };

  const res = await appSyncClient.mutate<api.CreateUserMutation>({
    mutation: gql(api.createUser),
    variables: vars
  });
  if (!res.data || !res.data.createUser) {
    throw new Error('no data found in response');
  }

  logger.info(`created user with id ${res.data.createUser.id}`);

  return res.data.createUser.id;
};

const updateCognitoUser = async (
  username: string,
  userID: string
): Promise<void> => {
  const updateCommand = new AdminUpdateUserAttributesCommand({
    UserPoolId: cognitoUserPoolID,
    Username: username,
    UserAttributes: [
      {
        Name: userIDClaim,
        Value: userID
      }
    ]
  });
  await idpClient.send(updateCommand);

  const userGroupCommand = new AdminAddUserToGroupCommand({
    UserPoolId: cognitoUserPoolID,
    Username: username,
    GroupName: 'user'
  });
  await idpClient.send(userGroupCommand);

  logger.info('updated cognito user');
};

const runUserCreate = async (
  data: IUserAttributes,
  username: string
): Promise<void> => {
  if (await userExists(data)) {
    throw new Error(`user already exists with email ${data.email}`);
  }

  const userID = await userCreate(data, username);
  await updateCognitoUser(username, userID);
};

export const handler: PostConfirmationTriggerHandler = async (
  event,
  _context,
  callback
) => {
  if (event.triggerSource === 'PostConfirmation_ConfirmForgotPassword') {
    // don't do anything for forgot password confirm
    return callback(null, event);
  }

  await initializeConfigFunction();

  try {
    await runUserCreate(
      event.request.userAttributes as unknown as IUserAttributes,
      event.userName
    );
  } catch (err) {
    const errObj = err as Error;
    return callback(errObj);
  }
  return callback(null, event);
};

const run = async (): Promise<void> => {
  await initializeConfigFunction();

  await runUserCreate(
    {
      email: 'admin@flexapp.ai',
      name: 'admin'
    },
    'admin@flexapp.ai'
  );
};

if (require.main === module) {
  run()
    .then(() => {
      logger.info('done with creating user');
      process.exit(0);
    })
    .catch((err: Error) => {
      // eslint-disable-next-line no-console
      console.error(err.message);
      process.exit(1);
    });
}

export default runUserCreate;
