import handlers.hello_pb2_grpc as hello_services
import pytest


pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.grpc',
]


@pytest.fixture
def grpc_service(grpc_channel, service_client):
    return hello_services.HelloServiceStub(grpc_channel)
