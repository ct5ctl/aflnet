import struct

# 准备报文内容
message_sequence = "DESCRIBE rtsp://127.0.0-1:8554/ac3AudioTest RTSP/1.0\r\nCSeq:\t2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streeaming Media v2018.08�28)\r\nSession: 000022B8\r\n\r\nDESCRIBE rtsp://127.0.0-1:8554/ac3AudioTest RTSP/1.0\r\nCSeq:\t2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streeaming Media v2018.08�28)\r\nSession: 00002\u0010B8\r\n\r\nDESCRIBE rtsp://127.0.0-1:8554/ac3AudioTest RTSP/1.0\r\nCSeq:\t2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streeaming Media v2018.08�28)\r\nSession: 000022B8\r\n\r\nSETUP "
# 将字符串转换为二进制
message_sequence_bytes = message_sequence.encode('utf-8', errors='replace')

# 分割报文序列成多个packet
packets = message_sequence_bytes.split(b"\r\n\r\n")

# 输出文件路径
output_file_path = './cc-test-queue/packet_fileb406'

# 打开文件以二进制写入
with open(output_file_path, "wb") as f:
    for packet in packets:
        if packet:  # 确保packet非空
            packet += b"\r\n\r\n"  # 补回分隔符
            # 写入报文的大小和内容
            f.write(struct.pack("I", len(packet)))
            f.write(packet)
