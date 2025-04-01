# Use: python3 server.py [Name of server]
import asyncio
import aiohttp
import json
import argparse
import time
import logging

API_KEY = "Put the key you obtain here" #From new Google Places API

server_ports = {
    "Bailey" : 10000, #12416,
    "Bona" : 10001, #12447,
    "Campbell" : 10002, #12478,
    "Clark" : 10003, #12509,
    "Jaquez": 10004 #12540
}

server_connections = {
    "Clark" : ["Jaquez", "Bona"],
    "Campbell" : ["Bailey", "Bona", "Jaquez"],
    "Bona" : ["Bailey", "Clark", "Campbell"],
    "Bailey" : ["Bona", "Campbell"],
    "Jaquez" : ["Clark", "Campbell"]
}

class Server:
    def __init__(self, person, ipaddr, portnum):
        self.person = person
        self.ipaddr = ipaddr
        self.portnum = portnum
        self.client_timestamp = {} # Client -> last time Client sent msg to us
        self.client_msg = {} # Client -> Server's last reply to client

    async def process_message(self, reader, writer): # Handle outside reader/writer objects
        try:
            while not reader.at_eof():
                # Get and log message
                msg = (await reader.readline()).decode()
                if len(msg) == 0:
                    continue
                logging.info(f"I received: {msg}")

                # Process contents
                reply = str()
                words = msg.split()
                if len(words) == 4:
                    if (words[0] == "IAMAT") and self.is_valid_IAMAT(words):
                        timediff = time.time() - float(words[3])
                        diffstring = str()
                        if timediff >= 0:
                            diffstring = "+" + str(timediff)
                        else:
                            diffstring = "-" + str(timediff)
                        reply = f"AT {self.person} {diffstring} {words[1]} {words[2]} {words[3]}"
                        self.client_msg[words[1]] = reply
                        self.client_timestamp[words[1]] = float(words[3])
                        await self.flood_message(reply)
                    elif (words[0] == "WHATSAT") and self.is_valid_WHATSAT(words):
                        client_requested = words[1]
                        rad = words[2]
                        inf_upbd = words[3]
                        last_msg = self.client_msg[client_requested]
                        location = last_msg.split()[4]
                        places = await self.places_near_location(location, rad, inf_upbd)
                        places = json.dumps(places, indent=4).rstrip('\n')
                        reply = f"{self.client_msg[client_requested]}\n{places}\n\n" 
                    else:
                        reply = "? " + msg
                elif (len(words) == 6) and words[0] == "AT":
                    server_talking_to_us = words[1]
                    client = words[3]
                    timestamp = float(words[5])
                    logging.info(f"{server_talking_to_us} flooded info about {client} to us.")
                    reply = None
                    if client in self.client_timestamp.keys():
                        if(timestamp > self.client_timestamp[client]): # Our info is outdated
                            logging.info(f"Updating data about {client} and flooding updates")
                            self.client_timestamp[client] = timestamp
                            self.client_msg[client] = msg
                            await self.flood_message(msg)
                        else: # Old news, we have newer info
                            logging.info(f"Received outdated info from {server_talking_to_us} about {client}, disregarding")
                    else:
                        logging.info(f"Learned about {client} for first time, adding to database and flooding")
                        self.client_timestamp[client] = timestamp
                        self.client_msg[client] = msg
                        await self.flood_message(msg)
                else:
                    reply = "? " + msg

                # Reply back
                if reply != None:
                    logging.info(f"Replying with: {reply}")
                    writer.write(reply.encode())
                    await writer.drain()
        finally:
            writer.close()                        
    
    def is_valid_IAMAT(self, words): #3rd arg in ISO 6709 notation, 4th arg POSIX time
        coords = words[2]
        if coords[0] not in "+-":
            return False
        
        coords = coords[1:].replace('-', '+')
        coord_values = coords[1:].split('+') 
        if (len(coord_values) != 2) or (not self.is_num(coord_values[0])) or (not self.is_num(coord_values[1])):
            return False
        
        if not self.is_num(words[3]):
            return False
        return True
    
    def is_valid_WHATSAT(self, words): # Valid client to ask about, 3rd and 4th args within bounds
        if words[1] not in self.client_msg.keys():
            return False # We don't have that client stored
        
        if (not self.is_num(words[2])) or (not self.is_num(words[3])):
            return False
        radius = float(words[2])
        info_upperbound = int(words[3])
        if (radius < 0) or (radius > 50):
            return False
        if (info_upperbound < 0) or (info_upperbound > 20):
            return False
        
        return True
    
    async def places_near_location(self, location, rad, info_upbd):
        async with aiohttp.ClientSession() as csesh:
            latitude, longitude = self.get_lat_and_long(location)
            url = 'https://places.googleapis.com/v1/places:searchNearby'
            headers = {
                'Content-Type': 'application/json',
                'X-Goog-Api-Key': API_KEY,
                'X-Goog-FieldMask': "*",
            }
            data = {
                "maxResultCount" : int(info_upbd),
                "locationRestriction" : {
                    "circle" : {
                        "center" : {
                            "latitude" : latitude,
                            "longitude" : longitude
                        },
                        "radius" : float(rad) }}}
            
            async with csesh.post(url, headers=headers, json=data) as response:
                if response.status == 200:
                    res = await response.json()
                    return res
                else:
                    logging.info(f"HTTP Error: {response.status}")
                    return None

    def get_lat_and_long(self, location):
        # +34.068930-118.445127 into nice list format for JSON and API call [34.068930, -118.445127]
        first = "" if (location[0] == '+') else "-"
        location = location[1:]
        plus_index = location.find('+')
        minus_index = location.find('-')
        if plus_index != -1:
            latitude = float(first + location[0:plus_index])
            longitude = float(location[plus_index + 1:])
        elif minus_index != -1:
            latitude = float(first + location[0:minus_index])
            longitude = float(location[minus_index:])

        return [latitude, longitude]
    
    async def flood_message(self, msg):
        for connection in server_connections[self.person]:
            try:
                reader, writer = await asyncio.open_connection("127.0.0.1", server_ports[connection])
                writer.write(msg.encode())
                logging.info(f"I sent {msg} to {connection}")
                await writer.drain()
                writer.close()
                await writer.wait_closed()
            except:
                logging.info(f"Error with TCP connection to {connection}")

    async def run(self):
        logging.info(f"I, {self.person}, am starting")
        srv = await asyncio.start_server(self.process_message, self.ipaddr, self.portnum)
        async with srv:
            await srv.serve_forever()
        
        logging.info(f"{self.person} shutting down")
        srv.close()

    def is_num(self, str):
        try:
            float(str)
            return True
        except ValueError:
            return False



def main():
    parser = argparse.ArgumentParser('server.py args')
    parser.add_argument('name', type=str)
    args = parser.parse_args()
    logging.basicConfig(
        filename=f"{args.name}.log",
        filemode='w+', 
        level=logging.INFO 
    )
    if args.name not in server_ports.keys():
        return 1
    srv = Server(args.name, "127.0.0.1", server_ports[args.name])
    try:
        asyncio.run(srv.run())
    except KeyboardInterrupt:
        return 0
        
if __name__ == '__main__':
    main()
   