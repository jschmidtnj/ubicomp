#!/usr/bin/env python3
"""
lambda handler for application analyze
"""

import boto3
import fitz
import json
from os import path, sep
from pdf2image import convert_from_bytes
from boto3_type_annotations.s3 import ServiceResource as S3Service
from boto3_type_annotations.cognito_idp import Client as CognitoIDPClient
from io import BytesIO
from PIL import Image
from loguru import logger
from config import ADMIN_PASS, ADMIN_USER, APPSYNC_ENDPOINT, COGNITO_CLIENT_ID, COGNITO_USER_POOL_ID, DATA_BUCKET
from urllib.parse import unquote_plus

boto3.setup_default_session()

s3: S3Service = boto3.resource('s3')
idp: CognitoIDPClient = boto3.client("cognito-idp")


temp_file_name = 'tmp.pdf'


def get_graphql(file_path: str) -> str:
    """
    get graphql file data
    """
    curr_path = path.join(path.dirname(__file__), './graphql/', file_path)
    with open(curr_path, 'r', encoding='utf-8') as f:
        data = f.read()
    return data


def get_gql_client() -> GraphQLClient:
    """
    create graphql client

    from https://levelup.gitconnected.com/connect-to-aws-appsync-using-python-heres-how-806c702e0e18
    https://stackoverflow.com/questions/37336286/how-do-i-call-an-api-gateway-with-cognito-credentials-in-python
    https://stackoverflow.com/questions/62311285/authenticating-appsync-mutation-using-aws-cognito-user-pool-on-graphql-playgroun
    """
    auth_res = idp.admin_initiate_auth(
        UserPoolId=COGNITO_USER_POOL_ID,
        ClientId=COGNITO_CLIENT_ID,
        AuthFlow="ADMIN_NO_SRP_AUTH",
        AuthParameters={"USERNAME": ADMIN_USER,
                        "PASSWORD": ADMIN_PASS},
    )

    challenge_name_key = 'ChallengeName'
    if challenge_name_key in auth_res:
        challenge_name = auth_res[challenge_name_key]
        if challenge_name != 'NEW_PASSWORD_REQUIRED':
            raise RuntimeError(auth_res)

        idp.respond_to_auth_challenge(
            ClientId=COGNITO_CLIENT_ID, ChallengeName=challenge_name,
            ChallengeResponses={
                'userAttributes.name': 'admin',
                'USERNAME': ADMIN_USER,
                'NEW_PASSWORD': ADMIN_PASS
            },
            Session=auth_res['Session'])

        auth_res = idp.admin_initiate_auth(
            UserPoolId=COGNITO_USER_POOL_ID,
            ClientId=COGNITO_CLIENT_ID,
            AuthFlow="ADMIN_NO_SRP_AUTH",
            AuthParameters={"USERNAME": ADMIN_USER,
                            "PASSWORD": ADMIN_PASS},
        )

    id_token = auth_res['AuthenticationResult']['IdToken']

    transport = RequestsHTTPTransport(
        url=APPSYNC_ENDPOINT, headers={
            'Authorization': id_token,
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        })
    client = GraphQLClient(transport=transport)
    return client


def get_output_key(input_key: str) -> str:
    """
    get key for output
    """
    if input_key == temp_file_name:
        return temp_file_name

    private_folder = 'private'
    applications_folder = 'applications'
    output_folder = 'output'

    cognito_identity = input_key.split(sep)[1]
    file_name = path.basename(input_key)
    output_key = path.join(
        private_folder, cognito_identity, applications_folder, output_folder, file_name)
    return output_key


def update_refs() -> None:
    """
    update refs for codegen
    """
    Form.update_forward_refs()


update_refs()


def process_file(input_key: str) -> str:
    """
    process application file
    """
    output = fitz.open()

    input_file = s3.Object(DATA_BUCKET, input_key)
    input_data = input_file.get()['Body'].read()
    input_pages = convert_from_bytes(input_data)

    form_id = path.splitext(path.basename(input_key))[0]

    form = Form(id=form_id, name=form_id, fields=[])

    for page_index, current_page in enumerate(input_pages):
        with BytesIO() as f:
            current_page.save(f, format="jpeg")
            f.seek(0)
            image_page = Image.open(f)
            dimensions = fitz.Rect(0, 0, image_page.width, image_page.height)
            processed_pdf, page_fields = get_processed(image_page)
            for field in page_fields:
                field.location.page = page_index
            form.fields.extend(page_fields)
            image_page.close()
        single_page_pdf = fitz.open("pdf", processed_pdf)
        output_page = output.new_page(width=dimensions.width,
                                      height=dimensions.height)
        output_page.show_pdf_page(dimensions, single_page_pdf, 0)

    output_data = output.write()

    output_key = get_output_key(input_key)
    json_data = json.dumps(form.dict())

    gql_client = get_gql_client()
    mutation = get_graphql('createForm.graphql')
    gql_client.execute(gql(mutation), variable_values=json_data)

    logger.info(json_data)

    output_object = s3.Object(DATA_BUCKET, output_key)
    output_object.put(Body=output_data)
    return output_key


def lambda_handler(event, _context) -> str:
    """
    handler for aws lambda
    """
    key = unquote_plus(event['Records'][0]['s3']
                       ['object']['key'], encoding='utf-8')
    output_key = process_file(key)
    return f'wrote file to {output_key}'


@logger.catch(reraise=True)
def main():
    """
    main function
    """
    input_file = s3.Object(DATA_BUCKET, temp_file_name)
    input_file.upload_file("applications/15 William Street Condo Renewal.pdf")

    output_id = path.splitext(path.basename(temp_file_name))[0]
    client = get_gql_client()
    get_form = get_graphql('getForm.graphql')
    res = client.execute(gql(get_form), variable_values=json.dumps({
        'formID': output_id
    }))
    if res['getForm']['id'] is not None:
        delete_form = get_graphql('deleteForm.graphql')
        client.execute(gql(delete_form), variable_values=json.dumps({
            'formID': output_id
        }))

    output_key = process_file(temp_file_name)
    output_object = s3.Object(DATA_BUCKET, output_key)
    output_object.download_file("output.pdf")
    logger.info(f'wrote file to {output_key}')

    list_forms = get_graphql('listForms.graphql')
    res = client.execute(gql(list_forms))
    logger.info(res)


if __name__ == "__main__":
    main()
