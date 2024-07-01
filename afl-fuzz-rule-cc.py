import json
import os
from datetime import datetime


# Define a global variable for table_rule
table_rule = {}

# import openai
# # 设置OpenAI API密钥
# openai.api_key = 'sk-xxx'


def process_message_sequences(q1, s1, q2, s2):
    # 计算报文数
    messages_q1 = q1.split("\r\n\r\n")
    messages_q2 = q2.split("\r\n\r\n")
    
    n_q1 = len(messages_q1) 
    n_q2 = len(messages_q2)
    # print("报文数:", n_q1, n_q2)
    
    # 遍历s1和s2，找到不同的位置
    pos_diff_status = []
    json_diff_queue_status = []
    
    for i in range(min(len(s1), len(s2), n_q1, n_q2)):
        # print("---------------------")
        if s1[i] != s2[i]:
            pos_diff_status.append(i)
            # print("---------------------")
            json_diff_queue_status.append({
                "报文对位置": i-1,
                "q1": {
                    "变异前报文": messages_q1[i-1],
                    "变异前状态": s1[i]
                },
                "q2": {
                    "变异后报文": messages_q2[i-1],
                    "变异后状态": s2[i]
                }
            })
    
    return pos_diff_status, json_diff_queue_status


def analyze_rtsp_mutation(q1, s1, q2, s2, key_variant_message):
    # Function to generate prompt based on input
    def generate_prompt(q1, s1, q2, s2, key_variant_message):
        prompt = f"""
        [q1]:"{q1}"

        [s1]:{s1}

        [q2]:"{q2}"

        [s2]:{s2}

        [关键变异报文对Key_variant_message]："{key_variant_message}"

        我正在对RTSP协议进行协议模糊测试，报文序列[q2]由报文序列[q1]变异而来，[q1]到达的状态序列为[s1]，[q2]到达的状态序列为[s2]，[关键变异报文对Key_variant_message]是变异后发生状态转换的报文对，请你对[关键变异报文对Key_variant_message]中的各报文对按以下步骤进行分析：

        1. 找出从q1到q2报文的变异位段，各变异位段通过其与报文字段的相对位置来定位（偏移、长度）
          注：变异位段是指两报文不同部分的具体位置，而不是不同部分所在的整个字段
        2. 请你对这些变异位段进行分析，筛选出有可能导致协议状态由[变异前状态]转换到[变异后状态]的变异位段，分别对这些变异位段的状态转换重要性进行评分
        3. 这些变异位段是否有其他变异方法可以导致相同的状态转换，给出变异后的该片段
        4. 运用你已有的触发协议漏洞相关知识，给出这些变异位段可以如何变异以导致协议崩溃，给出变异后的该片段

        注意：报文序列包含若干个报文，且报文有可能不为完整的格式，请以分隔符"\\r\\n\\r\\n"区分前后不同报文

        你的回答需要以json为输出格式，格式如下：

        {{
          # 描述变异类型
          "Difference1": "XXX",
          "变异类型":"XXX",
          "变异前状态":XXX,
          "变异后状态":XXX,
          # 描述变异位段位置
          "报文对位置":XXX,
          "q1变异位段位置信息":  
          {{"变异位段内容":"XXX", 
          "报文请求方法":"XXX", #变异位段所在报文的请求方法
          "报文字段名":"XXX", #变异位段所在字段名，使用该协议标准字段名
          "报文字段偏移":XX, #变异位段在所在字段的偏移
          "变异位段长度":XX, }},
          "q2变异位段位置信息":
          {{"变异位段内容":"XXX",
          "报文请求方法":"XXX", 
          "报文字段":"XXX",
          "报文字段偏移":XX,
          "变异位段长度":XX, }},
          # 评价该变异位段对状态转换重要性，若变异该片段更可能导致状态转换则给出更高分数，并给出打分的依据/原因
          "ImportanceScore": XX,
          "打分依据/原因":"",
          # 分析推断该片段还可以如何变异能导致协议状态转换，直接给出变异后的该片段和变异后可能到达的状态（状态用数字表示），可以有多个
          "OtherMutationMethods": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
                        {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
                        {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}],
          "可能导致崩溃的变异方法": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX","为什么会导致奔溃":"XXX"}},
                        {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX","为什么会导致奔溃":"XXX"}},
                        {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}]	
        }},
        {{
          "Difference2": "XXX",
          #格式同上
        }}
        """
        return prompt

    # Generate prompt
    prompt = generate_prompt(q1, s1, q2, s2, key_variant_message)
    print("prompt:  ", prompt)


    # Call GPT-4 with the generated prompt
    response = openai.Completion.create(
        model="text-davinci-003",
        prompt=prompt,
        max_tokens=1500,
        temperature=0.7
    )

    # # 模拟响应
    # response = {
    #     "choices": [
    #         {
    #             "text": """
    #             [
    #               {
    #                 "Difference1": "XXX",
    #                 "变异类型": "XXX",
    #                 "变异前状态": 200,
    #                 "变异后状态": 404,
    #                 "报文对位置": 0,
    #                 "q1变异位段位置信息": {
    #                   "变异位段内容": "SETUP",
    #                   "报文请求方法": "SETUP",
    #                   "报文字段名": "请求行",
    #                   "报文字段偏移": 0,
    #                   "变异位段长度": 5
    #                 },
    #                 "q2变异位段位置信息": {
    #                   "变异位段内容": "rrrrr",
    #                   "报文请求方法": "SETUP",
    #                   "报文字段": "请求行",
    #                   "报文字段偏移": 0,
    #                   "变异位段长度": 5
    #                 },
    #                 "ImportanceScore": 10,
    #                 "打分依据/原因": "请求方法变异导致状态变化",
    #                 "OtherMutationMethods": [
    #                   {
    #                     "变异1": "sssss",
    #                     "变异依据/原因": "修改方法名称",
    #                     "可能转换到的状态": "404",
    #                     "为什么转换到该状态": "未知请求方法"
    #                   }
    #                 ],
    #                 "可能导致崩溃的变异方法": [
    #                   {
    #                     "变异1": "xxxxxx",
    #                     "变异依据/原因": "插入无效字符",
    #                     "可能转换到的状态": "崩溃",
    #                     "为什么转换到该状态": "无法解析报文"
    #                   }
    #                 ]
    #               }
    #             ]
    #             """
    #         }
    #     ]
    # }
    # # 模拟保存响应
    # gpt_response = response["choices"][0]["text"].strip()

    # 解析响应
    gpt_response = response["choices"][0]["text"].strip()

    # Save the response to a JSON file
    timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
    filename = f'./cc-rule-base/answer/answer-{timestamp}.json'
    with open(filename, 'w', encoding='utf-8') as file:
        json.dump(json.loads(gpt_response), file, ensure_ascii=False, indent=4)

    print(f'Response saved to {filename}')

    return gpt_response



# New function to process the information and update table_rule
def update_table_rule(q1, s1, q2, s2, key_variant_message):
    global table_rule

    # Call the analyze_rtsp_mutation function
    gpt_response = analyze_rtsp_mutation(q1, s1, q2, s2, key_variant_message)
    gpt_data = json.loads(gpt_response)

    for diff in gpt_data:
        pre_state = diff["变异前状态"]
        post_state = diff["变异后状态"]
        importance_score = diff["ImportanceScore"]
        q1_mutation_info = diff["q1变异位段位置信息"]
        other_mutation_methods = diff["OtherMutationMethods"]

        # Ensure there's an entry for the target state
        if post_state not in table_rule:
            table_rule[post_state] = []

        # Extract relevant information and update table_rule
        entry = {
            "原状态": pre_state,
            "变异位段位置及相应变异方法": {
                "变异位段内容": q1_mutation_info["变异位段内容"],
                "报文请求方法": q1_mutation_info["报文请求方法"],
                "报文字段名": q1_mutation_info["报文字段名"],
                "报文字段偏移": q1_mutation_info["报文字段偏移"],
                "变异位段长度": q1_mutation_info["变异位段长度"],
                "其他变异方法": other_mutation_methods
            },
            "ImportanceScore": importance_score
        }

        # Add the entry to the target state's mutation table
        table_rule[post_state].append(entry)
    # Save table_rule to a file
    with open('table_rule.json', 'w', encoding='utf-8') as f:
        json.dump(table_rule, f, ensure_ascii=False, indent=4)

def Update_mutation_rule_table(q1, s1, q2, s2):
    # 运行函数
    pos_diff_status, key_variant_message = process_message_sequences(q1, s1, q2, s2)
    # print("key_variant_message:  ", key_variant_message)

    update_table_rule(q1, s1, q2, s2, key_variant_message)
    # Print the updated table_rule for verification
    print(json.dumps(table_rule, ensure_ascii=False, indent=4))



if __name__ == "__main__":
    # 示例数据
    # #t1
    # q1 = "DESCRIBE rtsp://127.0.0\u001d1:8554/wavAudioTest RTSP/1.0\r\nCSeq:\t2\r\nrser-Agent: ./tes[RTSPClient (LIVE555 Streeaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTEARDOWN rtsO://127.0.0.1:8554/wavAudioTest/(RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: .PAUSERTSPClientGET_PARAMETER (LIVE555 Sdreamitg Media v2018.08.28)\r\nSessi�n: "
    # s1 = [200, 454, 454, 400, 200, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 454, 404, 405, 400, 400, 454, 200, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 400, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 405, 404, 405, 400, 400, 454, 454, 200, 200, 200, 454, 454, 405, 400, 405, 454, 200, 405, 200, 200, 404, 404, 400, 200]
    # # t3
    # q2 = "DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept= application v20Q8.0DES rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTAgent: ./testRTSPClient (LIVE555 Etreaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTE,RDOSETUPWN rtsPAUS<pR//127.0.0.1e8d54/wavAudgPLAYoTest/ RTSP/1�0\r�CSeq: \r\nUng PTIONSiov2018.08.28)\r\nAccept:\u000f0NSiov2018K22B8\r\n\r\nTEARDOWN rtsp://127.0.CCCCCCCCCCCCCCCCCCCCCCCCTSP/1.0\r\nCSeq: 5\r\nUser-AgeLt: ./tesZRTS�Client (LIVE555 Samingac3OudioTest.08.28)\r\nSession: 000022;8\r\n\r\nDESCRIBE rtsp:/�\u000327�.0.1:8554/wavAud1:8554/wavA8554/wavAudioioTest RCSe"
    # s2 = [200, 405, 400, 400, 454, 200, 200, 200, 400, 454, 200, 405, 200, 200, 404, 404]
    # # t2
    # q2 = "DESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3(\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unic`st;client_port=37952-37953\r\n#\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE55\u0013 Streaming Media v2018.08.28)\r\nTranspnit: RTP/AVP;unicast;client_port=3795tsp://12nge: npt=0.000-\r\n\r\nDst RTQP/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./testRTSPClient (LIVE555 Streaming Media v20?\u0018.08'28)\r\nAccept: a�Transport/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGeT_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSess�on: 000022B8\r\nRange6 npt=0.000-\r\n\r\nDESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClSET_PARAMETERient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming55 St Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unicast;client_port=37952-37953\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSs/1.0\r\nCSeq: 5\r\n\\ser-Agent: ./tesOPTIO\u0010StRTSPClient (LIVE555 Streaming Media v2018.08�"
    # s2 = [400, 454, 454, 200, 400, 454, 400, 454, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 400, 400, 200, 200, 200, 400, 454, 400, 454, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 404, 405, 400, 400, 400, 200, 400, 200, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200]
    # # t4
    # q1 = "SETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1\f0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPCl�ent (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;uni�ast;client_port=374444444444444444444444444444444444444444444444444I4444444444444444E44444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444!444444.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nDESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Stre�aming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSSClient (LIVE55\u0016 Strevming Media v2018.08.28)\r\nSession: 000022B8\r\nRanme: npt=matroskaFileTest /127.0.0.1:8454/wavAudioTezt/ RTSP/1.0\r\nCSeq: 4\r\nUs�r-Agen "
    # s1 = [0, 200, 200, 200, 400, 200, 405, 200, 200, 400, 200, 405, 400, 400, 404, 400, 200, 405, 400, 200, 404, 400, 400, 404, 400, 200, 404, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400]
    # # t4人工变异
    # q2 = "SETUP rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrtsp://////////////////////////////////127.0.0.1:8554////////////wavAudioTest/track1 RTSP/1\f0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPCl�ent (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;uni�ast;client_port=)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nDESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Stre�aming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSSClient (LIVE55\u0016 Strevming Media v2018.08.28)\r\nSession: 000022B8\r\nRanme: npt=matroskaFileTest /127.0.0.1:8454/wavAudioTezt/ RTSP/1.0\r\nCSeq: 4\r\nUs�r-Agen "
    # s2 = [0, 404, 200, 405]

    # 示例数据列表
    mutation_data = [
        {
            "kl_messages": "TP/\u0013.q\u001a 5\r\nSETUPUser-Agent: ./testRTSPCdieAudioTuqt\u000f RMSP/1.0\r\n_Seq: 5\r\nU\u0001",
            "state_sequence": [0, 400, 400, 400, 200, 400, 400, 405, 200, 405, 405, 454, 405, 400, 405, 405, 200, 405, 405, 454, 454, 405, 405, 400, 405, 400, 405, 400, 400, 405, 200, 405, 405, 454, 405, 400, 405, 405, 200, 405, 405, 454, 454, 405, 405, 400, 405, 400, 405, 400],
            "state_count": 50
        },
        {
            "kl_messages": "DESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3(\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unic`st;client_port=37952-37953\r\n#\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE55\u0013 Streaming Media v2018.08.28)\r\nTranspnit: RTP/AVP;unicast;client_port=3795tsp://12nge: npt=0.000-\r\n\r\nDst RTQP/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./testRTSPClient (LIVE555 Streaming Media v20?\u0018.08'28)\r\nAccept: a�Transport/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGeT_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSess�on: 000022B8\r\nRange6 npt=0.000-\r\n\r\nDESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClSET_PARAMETERient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming55 St Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unicast;client_port=37952-37953\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSs/1.0\r\nCSeq: 5\r\n\\ser-Agent: ./tesOPTIO\u0010StRTSPClient (LIVE555 Streaming Media v2018.08�",
            "state_sequence": [0, 400, 454, 454, 200, 400, 454, 400, 454, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 404, 405, 400, 400, 400, 200, 400, 200, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200],
            "state_count": 135
        },
        # 添加更多的变异数据...
    ]
    for data in mutation_data:
        q1 = data["kl_messages"]
        s1 = data["state_sequence"]
        q2 = data["kl_messages"]  # 使用变异后的消息作为q2，s2
        s2 = data["state_sequence"]

        Update_mutation_rule_table(q1, s1, q2, s2)


    


    