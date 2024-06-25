import json

def process_message_sequences(q1, s1, q2, s2):
    # 计算报文数
    messages_q1 = q1.split("\r\n\r\n")
    messages_q2 = q2.split("\r\n\r\n")
    
    n_q1 = len(messages_q1) 
    n_q2 = len(messages_q2)
    print("报文数:", n_q1, n_q2)
    
    # 遍历s1和s2，找到不同的位置
    pos_diff_status = []
    json_diff_queue_status = []
    
    for i in range(min(len(s1), len(s2), n_q1, n_q2)):
        print("---------------------")
        if s1[i] != s2[i]:
            pos_diff_status.append(i)
            print("---------------------")
            json_diff_queue_status.append({
                "报文对位置": i,
                "q1": {
                    "变异前报文": messages_q1[i],
                    "变异前状态": s1[i]
                },
                "q2": {
                    "变异后报文": messages_q2[i],
                    "变异后状态": s2[i]
                }
            })
    
    return pos_diff_status, json_diff_queue_status

# 示例数据
q1 = "DESCRIBE rtsp://127.0.0\u001d1:8554/wavAudioTest RTSP/1.0\r\nCSeq:\t2\r\nrser-Agent: ./tes[RTSPClient (LIVE555 Streeaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTEARDOWN rtsO://127.0.0.1:8554/wavAudioTest/(RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: .PAUSERTSPClientGET_PARAMETER (LIVE555 Sdreamitg Media v2018.08.28)\r\nSessi�n: "
s1 = [200, 454, 454, 400, 200, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 454, 404, 405, 400, 400, 454, 200, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 400, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 405, 404, 405, 400, 400, 454, 454, 200, 200, 200, 454, 454, 405, 400, 405, 454, 200, 405, 200, 200, 404, 404, 400, 200]
q2 = "DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept= application v20Q8.0DES rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTAgent: ./testRTSPClient (LIVE555 Etreaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTE,RDOSETUPWN rtsPAUS<pR//127.0.0.1e8d54/wavAudgPLAYoTest/ RTSP/1�0\r�CSeq: \r\nUng PTIONSiov2018.08.28)\r\nAccept:\u000f0NSiov2018K22B8\r\n\r\nTEARDOWN rtsp://127.0.CCCCCCCCCCCCCCCCCCCCCCCCTSP/1.0\r\nCSeq: 5\r\nUser-AgeLt: ./tesZRTS�Client (LIVE555 Samingac3OudioTest.08.28)\r\nSession: 000022;8\r\n\r\nDESCRIBE rtsp:/�\u000327�.0.1:8554/wavAud1:8554/wavA8554/wavAudioioTest RCSe"
s2 = [200, 405, 400, 400, 454, 200, 200, 200, 400, 454, 200, 405, 200, 200, 404, 404]

# 运行函数
pos_diff_status, json_diff_queue_status = process_message_sequences(q1, s1, q2, s2)

# 打印结果
print("不同位置:", pos_diff_status)
print("不同报文状态:", json.dumps(json_diff_queue_status, ensure_ascii=False, indent=4))
