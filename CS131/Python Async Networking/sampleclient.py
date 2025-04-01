# Use: python3 sampleclient.py [Server name to connect to]
import asyncio
import argparse

server_ports = {
    "Bailey" : 10000, #12416,
    "Bona" : 10001, #12447,
    "Campbell" : 10002, #12478,
    "Clark" : 10003, #12509,
    "Jaquez": 10004 #12540
}

class Client:
    def __init__(self, srvname):
        self.ip = "127.0.0.1"
        self.srvname = srvname
        self.srvport = server_ports[srvname]

    async def communicate_with_server(self, message):
        reader, writer = await asyncio.open_connection(self.ip, self.srvport)
        print(f"Sending message to {self.srvname}")
        writer.write(message.encode())

        data = await reader.read(100000000) 
        if data != None:
            print(f"Received message back from {self.srvname}")
        writer.close()

    def run(self):
        while True:
            message = input(f"Enter input to send to {self.srvname}: ")
            message += '\n'
            asyncio.run(self.communicate_with_server(message))

def main():
    parser = argparse.ArgumentParser('sampleclient.py args')
    parser.add_argument('srvname', type=str)
    args = parser.parse_args()
    client = Client(args.srvname)
    client.run()

if __name__ == '__main__':
    main()
