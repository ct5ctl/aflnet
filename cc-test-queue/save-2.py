import struct

# 准备报文内容
packet1 = b"TEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\n"
packet2 = b" StreamiKg Media v201PAUSE8.08.28)\r\nAccept: application/sdp\r\n\r\n"
packet3 = b"SETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: GTP/AVP;unicast;clreaming Mediient_port=37952-379@3\r\n\r\n"
packet4 = b"PLAY rtsp://127.4/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\n"
packet5 = b"PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIp://127.0.0.1:8554/VE535 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.800-\rN rtsp://127.0;0.1:8554/wavAuviocest/ RTSP/1.0\r\nCSeq: 5\r\nUse\\-AgeAcceptnt: .ee/hestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\n"
packet6 = b"DESCRIBE rtsp://127.0:0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v20\u0004"

# packet1 = b"DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept: application/sdp\r\n\r\n"
# packet2 = b"DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSe"

# packet1 = b"TEARDOWN rtsp://127d0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:TSPClient (LIVE555(Streaming Media v55(Streaming Media v2018.08.28)\r����sion: 000022B8\r\n\r\n"
# packet2 = b"DESCRIBE rtsp:'/127.0.0.1:8554/wavAudioTest RTSP/1.0\r\n<Seq: 2\r\nYser-Agent: ./testac3AudioTe127.0.0.1stRTSPClient�LIVE555 Streaming�"

output_file_path = './cc-test-queue/packet_file'

# 打开文件以二进制写入
with open(output_file_path, "wb") as f:
    # 写入第一个报文的大小和内容
    f.write(struct.pack("I", len(packet1)))
    f.write(packet1)
    # 写入第二个报文的大小和内容
    f.write(struct.pack("I", len(packet2)))
    f.write(packet2)
    f.write(struct.pack("I", len(packet3)))
    f.write(packet3)
    f.write(struct.pack("I", len(packet4)))
    f.write(packet4)
    f.write(struct.pack("I", len(packet5)))
    f.write(packet5)
    f.write(struct.pack("I", len(packet6)))
    f.write(packet6)