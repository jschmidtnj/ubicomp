#!/usr/bin/env python3
"""
config file

reads configuration from environment
"""

from os import getenv
from dotenv import load_dotenv, find_dotenv
from typing import Optional
from loguru import logger

# aws
DATA_BUCKET: str = ''
COGNITO_CLIENT_ID: str = ''
COGNITO_USER_POOL_ID: str = ''
APPSYNC_ENDPOINT: str = ''

ADMIN_USER: str = 'admin@flexapp.ai'
ADMIN_PASS: str = ''

CONFIG_LOADED: bool = False
PRODUCTION: bool = True


def read_config() -> None:
    """
    read the config from environment
    """
    global DATA_BUCKET
    global APPSYNC_ENDPOINT
    global COGNITO_CLIENT_ID
    global COGNITO_USER_POOL_ID
    global CONFIG_LOADED
    global ADMIN_PASS
    global PRODUCTION

    if CONFIG_LOADED:
        logger.info('config already loaded')
        return

    load_dotenv(find_dotenv(usecwd=True), override=True)

    data_bucket: Optional[str] = getenv('DATA_BUCKET')
    if data_bucket is None:
        raise ValueError('data bucket is not defined')
    DATA_BUCKET = data_bucket

    cognito_client_id: Optional[str] = getenv('COGNITO_CLIENT_ID')
    if cognito_client_id is None:
        raise ValueError('cognito client id is not defined')
    COGNITO_CLIENT_ID = cognito_client_id

    cognito_user_pool_id: Optional[str] = getenv('COGNITO_USER_POOL_ID')
    if cognito_user_pool_id is None:
        raise ValueError('cognito user pool id is not defined')
    COGNITO_USER_POOL_ID = cognito_user_pool_id

    admin_pass: Optional[str] = getenv('ADMIN_PASS')
    if admin_pass is None:
        raise ValueError('admin password id is not defined')
    ADMIN_PASS = admin_pass

    appsync_endpoint: Optional[str] = getenv('APPSYNC_ENDPOINT')
    if appsync_endpoint is None:
        raise ValueError('app sync endpoint is not defined')
    APPSYNC_ENDPOINT = appsync_endpoint

    production: Optional[str] = getenv('PRODUCTION')
    if production is not None:
        PRODUCTION = production == 'true'
    CONFIG_LOADED = True
    logger.info('config finished loading')


# you always want to run read_config on import, so there shouldn't be a run catch block
read_config()
