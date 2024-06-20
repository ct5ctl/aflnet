import struct

# 准备报文内容
message_sequence = "SETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\nange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\nange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\nange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\nange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\n[ange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1\u00140\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\nange: npt=0.000-\r\n\r\nPLAY rts�://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq:$4\rPClient (LIVE555 Streaming Media�v201F.08.28)\r\nSession: 000022B8\r\n[ange: npt=0.000-\r\n\r\nTEARDOWV rtsp://127.0.0.1:8554/wavAudioTest/ RTSk/1.0\r\nCSeR: 5\r\nUse�-Agent: ./testRTSPClient (LIVEtreaming +edia v2018.08.2d"
# 将字符串转换为二进制
message_sequence_bytes = message_sequence.encode('utf-8', errors='replace')

# 分割报文序列成多个packet
packets = message_sequence_bytes.split(b"\r\n\r\n")

# 输出文件路径
output_file_path = './cc-test-queue/packet_fileb405'

# 打开文件以二进制写入
with open(output_file_path, "wb") as f:
    for packet in packets:
        if packet:  # 确保packet非空
            packet += b"\r\n\r\n"  # 补回分隔符
            # 写入报文的大小和内容
            f.write(struct.pack("I", len(packet)))
            f.write(packet)
