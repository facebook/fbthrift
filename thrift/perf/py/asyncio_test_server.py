# @lint-avoid-pyflakes2

from argparse import ArgumentParser
import asyncio
import sys

from thrift.server.TAsyncioServer import ThriftAsyncServerFactory

from apache.thrift.test.asyncio.asyncio_load_handler import LoadHandler


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--port',
        default=1234,
        type=int,
        help='Port to run on')
    options = parser.parse_args()
    loop = asyncio.get_event_loop()
    handler = LoadHandler()
    server = loop.run_until_complete(
        ThriftAsyncServerFactory(handler, port=options.port, loop=loop))
    print("Running Asyncio server on port {}".format(options.port))

    try:
        loop.run_forever()
    except KeyboardInterrupt:
        print("Caught SIGINT, exiting")
    finally:
        server.close()
        loop.close()

if __name__ == '__main__':
    sys.exit(main())
