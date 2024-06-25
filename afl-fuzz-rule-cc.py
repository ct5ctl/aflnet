import json
import json

import os
from datetime import datetime

# import openai
# # 设置OpenAI API密钥
# openai.api_key = 'your-api-key-here'


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


def analyze_rtsp_mutation(q1, s1, q2, s2, key_variant_message):
    # Function to generate prompt based on input
    def generate_prompt(q1, s1, q2, s2, key_variant_message):
        prompt = f"""
        q1:

        "{q1}"

        s1:

        "{json.dumps(s1)}"

        q2:

        "{q2}"

        s2:

        "{json.dumps(s2)}"

        {{"关键变异报文对Key_variant_message"}}：

        {json.dumps(key_variant_message, ensure_ascii=False)}

        我正在对RTSP协议进行协议模糊测试，报文序列{{q2}}由报文序列{{q1}}变异而来，{{q1}}到达的状态序列为{{s1}}，{{q2}}到达的状态序列为{{s2}}，{{关键变异报文对}}是变异后发生状态转换的报文对，请你对{{关键变异}}中的各报文对按以下步骤进行分析：

        1. 找出这些报文对的变异片段，变异片段长度应尽可能小（只是不同的部分，而不是整个报文或字段）
        2. 请你对这些变异片段进行分析，筛选出有可能导致协议状态由{{变异前状态}}转换到{{变异后状态}}的变异片段，分别对这些变异片段的状态转换重要性进行评分
        3. 这些变异片段还可以如何变异以导致相同的状态转换，给出变异后的该片段

        注意：报文序列包含若干个报文，且报文有可能不为完整的格式，请以分隔符”\r\n\r\n“区分前后不同报文

        你的回答需要以json为输出格式，格式如下：

        {
          "Difference1": "XXX",
          "变异类型":"XXX",
          "变异前状态":"XXX",
          "变异后状态":"XXX",
          "报文对位置":"XXX",
          "q1变异片段位置信息":  
          {{"变异片段内容":"XXX", 
          "报文请求方法":"XXX", 
          "报文字段":"XXX", 
          "报文字段偏移":"XXX", 
          "变异片段长度":"XXX", }},
          "q2变异片段位置信息":
          {{"变异片段内容":"XXX",
          "报文请求方法":"XXX",  
          "报文字段":"XXX", 
          "报文字段偏移":"XXX", 
          "变异片段长度":"XXX", }},
          "ImportanceScore": "XXX", 
          "打分依据/原因":"",
          "OtherMutationMethods": [{"变异1":"XXX","可能转换到的状态":"XXX"}, # 分析推断该片段还可以如何变异能导致协议状态转换，直接给出变异后的该片段和变异后可能到达的状态（状态用数字表示），可以有多个
                                  {"变异2":"XXX","可能转换到的状态":"XXX"},
                                  {"变异3":"XXX","可能转换到的状态":"XXX"}]
        },
        {
          "Difference2": "XXX", 
        }
        """
        return prompt

    # Generate prompt
    prompt = generate_prompt(q1, s1, q2, s2, key_variant_message)
    print("prompt:  ", prompt)

    # # Call GPT-4 with the generated prompt
    # response = openai.Completion.create(
    #     model="text-davinci-003",
    #     prompt=prompt,
    #     max_tokens=1500,
    #     temperature=0.7
    # )

    # # Save the response to a JSON file
    # timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
    # filename = f'./cc-rule-base/answer/answer-{timestamp}.json'
    # with open(filename, 'w', encoding='utf-8') as file:
    #     json.dump(response.choices[0].text.strip(), file, ensure_ascii=False, indent=4)

    # print(f'Response saved to {filename}')




if __name__ == "__main__":
    # 示例数据
    #t1
    q1 = "DESCRIBE rtsp://127.0.0\u001d1:8554/wavAudioTest RTSP/1.0\r\nCSeq:\t2\r\nrser-Agent: ./tes[RTSPClient (LIVE555 Streeaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTEARDOWN rtsO://127.0.0.1:8554/wavAudioTest/(RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: .PAUSERTSPClientGET_PARAMETER (LIVE555 Sdreamitg Media v2018.08.28)\r\nSessi�n: "
    s1 = [200, 454, 454, 400, 200, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 454, 404, 405, 400, 400, 454, 200, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 400, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 405, 404, 405, 400, 400, 454, 454, 200, 200, 200, 454, 454, 405, 400, 405, 454, 200, 405, 200, 200, 404, 404, 400, 200]
    # # t3
    # q2 = "DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept= application v20Q8.0DES rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTAgent: ./testRTSPClient (LIVE555 Etreaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTE,RDOSETUPWN rtsPAUS<pR//127.0.0.1e8d54/wavAudgPLAYoTest/ RTSP/1�0\r�CSeq: \r\nUng PTIONSiov2018.08.28)\r\nAccept:\u000f0NSiov2018K22B8\r\n\r\nTEARDOWN rtsp://127.0.CCCCCCCCCCCCCCCCCCCCCCCCTSP/1.0\r\nCSeq: 5\r\nUser-AgeLt: ./tesZRTS�Client (LIVE555 Samingac3OudioTest.08.28)\r\nSession: 000022;8\r\n\r\nDESCRIBE rtsp:/�\u000327�.0.1:8554/wavAud1:8554/wavA8554/wavAudioioTest RCSe"
    # s2 = [200, 405, 400, 400, 454, 200, 200, 200, 400, 454, 200, 405, 200, 200, 404, 404]
    # t2
    q2 = "DESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3(\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unic`st;client_port=37952-37953\r\n#\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE55\u0013 Streaming Media v2018.08.28)\r\nTranspnit: RTP/AVP;unicast;client_port=3795tsp://12nge: npt=0.000-\r\n\r\nDst RTQP/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./testRTSPClient (LIVE555 Streaming Media v20?\u0018.08'28)\r\nAccept: a�Transport/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGeT_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSess�on: 000022B8\r\nRange6 npt=0.000-\r\n\r\nDESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClSET_PARAMETERient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming55 St Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unicast;client_port=37952-37953\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSs/1.0\r\nCSeq: 5\r\n\\ser-Agent: ./tesOPTIO\u0010StRTSPClient (LIVE555 Streaming Media v2018.08�"
    s2 = [400, 454, 454, 200, 400, 454, 400, 454, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 400, 400, 200, 200, 200, 400, 454, 400, 454, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 404, 405, 400, 400, 400, 200, 400, 200, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200]

    # 运行函数
    pos_diff_status, key_variant_message = process_message_sequences(q1, s1, q2, s2)
    print("key_variant_message:  ", key_variant_message)

    # # 打印结果
    # print("不同位置:", pos_diff_status)
    # print("不同报文状态:", json.dumps(key_variant_message, ensure_ascii=False, indent=4))

    # key_variant_message = {
    #     "报文对位置": 1,
    #     "q1": {
    #         "变异前报文": "TEARDOWN rtsO://127.0.0.1:8554/wavAudioTest/(RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: .PAUSERTSPClientGET_PARAMETER (LIVE555 Sdreamitg Media v2018.08.28)\r\nSessi�n: ",
    #         "变异前状态": 454
    #     },
    #     "q2": {
    #         "变异后报文": "TE,RDOSETUPWN rtsPAUS<pR//127.0.0.1e8d54/wavAudgPLAYoTest/ RTSP/1�0\r�CSeq: \r\nUng PTIONSiov2018.08.28)\r\nAccept:\u000f0NSiov2018K22B8",
    #         "变异后状态": 405
    #     }
    # }

    # 调用函数
    # analyze_rtsp_mutation(q1, s1, q2, s2, key_variant_message)
    # print(result)