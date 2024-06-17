import struct

# 准备报文内容
packet1 = b"SETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\n"
packet2 = b"DESCRI:E rtsp://127.0.0.1:8\u001854/wav\r\n\r\n"

output_file_path = './cc-test-queue/packet_file'

# 打开文件以二进制写入
with open(output_file_path, "wb") as f:
    # 写入第一个报文的大小和内容
    f.write(struct.pack("I", len(packet1)))
    f.write(packet1)
    # 写入第二个报文的大小和内容
    f.write(struct.pack("I", len(packet2)))
    f.write(packet2)