# Define the packet data
packet_data = """SETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nDESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept: application/sdp\r\n\r\n"""

# Convert the packet data to binary
packet_data_binary = packet_data.encode('utf-8')

# Define the output file path
output_file_path = './cc-test-queue/packet_file'

# Write the binary data to the file
with open(output_file_path, 'wb') as file:
    file.write(packet_data_binary)

print(f"Packet data written to {output_file_path} in binary format.")
