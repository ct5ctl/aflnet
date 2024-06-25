import struct

# 准备报文内容
message_sequence = "DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept= application v20Q8.0DES rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/2.0\r\nCSeq: 5\r\nUser-Agent: ./testRTAgent: ./testRTSPClient (LIVE555 Etreaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTE,RDOSETUPWN rtsPAUS<pR//127.0.0.1e8d54/wavAudgPLAYoTest/ RTSP/1�0\r�CSeq: \r\nUng PTIONSiov2018.08.28)\r\nAccept:\u000f0NSiov2018K22B8\r\n\r\nTEARDOWN rtsp://127.0.CCCCCCCCCCCCCCCCCCCCCCCCTSP/1.0\r\nCSeq: 5\r\nUser-AgeLt: ./tesZRTS�Client (LIVE555 Samingac3OudioTest.08.28)\r\nSession: 000022;8\r\n\r\nDESCRIBE rtsp:/�\u000327�.0.1:8554/wavAud1:8554/wavA8554/wavAudioioTest RCSe"
# 将字符串转换为二进制
message_sequence_bytes = message_sequence.encode('utf-8', errors='replace')

# 分割报文序列成多个packet
packets = message_sequence_bytes.split(b"\r\n\r\n")

# 输出文件路径
output_file_path = './cc-test-queue/packet_filet3b_r12'

# 打开文件以二进制写入
with open(output_file_path, "wb") as f:
    for packet in packets:
        if packet:  # 确保packet非空
            packet += b"\r\n\r\n"  # 补回分隔符
            # 写入报文的大小和内容
            f.write(struct.pack("I", len(packet)))
            f.write(packet)
