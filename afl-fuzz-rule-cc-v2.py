import json
import os
from datetime import datetime
import subprocess
import sys

try:
    from orderedmultidict import omdict
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "orderedmultidict"])
    from orderedmultidict import omdict
try:
    import openai
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "openai"])
    import openai

# Define a global variable for table_rule using ordered multi-dict
table_rule = omdict()


# # 设置OpenAI API密钥    
openai.api_key = 'sk-xxx'
# openai.api_key = os.getenv("OPENAI_API_KEY")


def insert_commas_and_quotes(rtsp_sequence: str) -> str:
    """
    Insert a comma after each RTSP message in the sequence and enclose each message in double quotes.
    Messages are separated by the delimiter '\r\n\r\n'.
    """
    delimiter = "\r\n\r\n"
    messages = rtsp_sequence.split(delimiter)
    quoted_messages = [f'"{msg}"' for msg in messages]
    return delimiter.join([msg + "," for msg in quoted_messages])

def find_differences(s1, s2):
    # 确定比较的长度为两个列表中较短的那一个
    min_length = min(len(s1), len(s2))
    # 存储不同元素的索引
    differences = []
    
    # 遍历两个列表的前min_length个元素,比较它们
    for i in range(min_length):
        if s1[i] != s2[i]:
            differences.append(i)
    
    return differences


def analyze_rtsp_mutation(q1, s1, q2, s2):
    # Function to generate prompt based on input
    def generate_prompt(q1, s1, q2, s2):
        diff_pos = find_differences(s1, s2)
        result_string = ""
        for i in diff_pos:
            if result_string:  # 如果result_string不为空,加入逗号和空格进行分隔
                result_string += "、"
            # 根据索引从q1和q2中取值,格式化字符串
            result_string += f"M_before_muta[{i}]到M_after_muta[{i}]"
        # prompt = rf"""
        # M_before_muta:[{q1}]
        # S_before_muta:{s1}
        # M_after_muta:[{q2}]
        # S_after_muta:{s2}
        # 我正在对RTSP协议进行协议模糊测试。M_before_muta、M_before_muta、S_before_muta、S_after_muta均为列表,M_before_muta与M_before_muta为报文列表,M_before_muta由M_before_muta经过部分变异得到。S_before_muta为M_before_muta到达的状态列表,两者数量一一对应。S_after_muta为M_before_muta到达的状态列表,两者数量一一对应。
        # 将S_before_muta与S_after_muta进行比对后,发现S_before_muta[1]与S_after_muta[1]、S_before_muta[2]与S_after_muta[2]不同,由此推断出{result_string}的变异产生了状态转换,请关注这2组产生状态转换的关键报文对并按以下步骤进行分析:
        # 步骤1:找出每个关键报文对的所有变异区间,若1个报文对包含多个变异区间,需要赋予不同变异序号并分别进行分析
        # 步骤2:列出所有可能导致状态转换的变异区间并进行定位,这些变异区间通过其与所在报文序列的偏移量、长度等信息进行定位
        # 步骤3:根据各变异区间的变异对协议状态转换的重要性,分别对这些变异区间的"结构敏感性分数"进行评分,并给出打分的依据/原因
        # 步骤4:分析这些变异区间是否有不同于关键报文对的其他变异方法可以导致相同状态转换,给出变异后的协议片段 
        # 注意:
        # 1. 变异区间是报文对间不同点的最小单位,其长度只能小于或等于报文的最小单元（如请求报文的Method、URI、Version、CSeq、Content-Type、User-Agent、Content-Length、Entity Body、End-of-Header Marker等字段）,若变异区间长度大于报文最小单元,需要将其拆分为多个小变异区间分别进行分析
        # 2. 一个报文对若包含多个变异区间,需要拆分进行分别分析,每个变异区间用一个单独的变异区间序号进行标识
        # 3. 你必须完整地对所有关键报文对的所有变异区间进行分析
        # 4. 即使实际输出篇幅很长而超过单次回答字数上限,也不能省略任何分析结果。
        # 你的中间分析过程可以简略,但在分析完毕后,对所有变异区间的分析结果都要按如下json格式重新完整输出一遍:
        # ```
        # {{
        # "变异区间序号": XX,
        # "所述报文对": "M_before_muta[i]到M_before_muta[i]", 
        # "变异前状态":S_before_muta[i], #变异区间所在报文触发状态
        # "变异后状态":S_after_muta[i],
        # # 描述变异区间位置
        # "M_before_muta变异区间内容":"XXX", 
        # "M_after_muta变异区间内容":"XXX",
        # "M_before_muta变异区间定位信息":  
        # {{
        # "报文位置索引":XX, #变异区间所处报文在M_before_muta列表中的位置索引
        # "报文请求方法":"XXX", #变异区间所在报文的请求方法
        # "变异区间偏移":XX, #变异区间距离M_before_muta头部的偏移，以字节为单位
        # "变异区间长度":XX, }}, #即strlen(变异区间内容)，以字节为单位
        # "M_after_muta变异区间定位信息":
        # {{
        # "报文位置索引":XX, #变异区间所处报文在M_after_muta列表中的位置索引
        # "报文请求方法":"XXX", 
        # "变异区间偏移":XX,
        # "变异区间长度":XX, }},
        # "结构敏感性分数": XX,
        # "评分依据/原因":"",
        # # 分析推断该片段还可以如何变异能导致相同协议状态转换,给出变异后的片段，并给出变异后可能到达的状态（状态用数字表示）,可以有多个
        # "导致相同状态转换的其他变异方法": [{{"变异序号":1, "变异后片段":"XXX", "可能转换到的状态":"XXX"}},
        #                             {{"变异序号":2, "变异后片段":"XXX", "可能转换到的状态":"XXX"}},
        #                             {{"变异序号":3, "变异后片段":"XXX", "可能转换到的状态":"XXX"}}]
        # }},
        # {{
        # "变异区间序号": XX,
        # #格式同上
        # }}
        # ```
        # """

        prompt = rf"""
        M_before_muta:[{q1}]
        S_before_muta:{s1}
        M_after_muta:[{q2}]
        S_after_muta:{s2}
        我正在对RTSP协议进行协议模糊测试。M_before_muta、M_before_muta、S_before_muta、S_after_muta均为列表,M_before_muta与M_before_muta为报文列表,M_before_muta由M_before_muta经过部分变异得到。S_before_muta为M_before_muta到达的状态列表,两者数量一一对应。S_after_muta为M_before_muta到达的状态列表,两者数量一一对应。
        将S_before_muta与S_after_muta进行比对后,发现S_before_muta[1]与S_after_muta[1]、S_before_muta[2]与S_after_muta[2]不同,由此推断出{result_string}的变异产生了状态转换,请关注这2组产生状态转换的关键报文对,找出每个关键报文对的所有变异区间并按以下json格式进行分析和输出:
        ```
        {{
        "变异区间序号": XX,
        "所述报文对": "M_before_muta[i]到M_before_muta[i]", 
        "变异前状态":S_before_muta[i], #变异区间所在报文触发状态
        "变异后状态":S_after_muta[i],
        # 通过变异区间与所在报文序列的偏移量、长度等信息对变异区间进行定位
        "M_before_muta变异区间内容":"XXX", 
        "M_after_muta变异区间内容":"XXX",
        "M_before_muta变异区间定位信息":  
        {{
        "报文位置索引":XX, #变异区间所处报文在M_before_muta列表中的位置索引
        "报文请求方法":"XXX", #变异区间所在报文的请求方法
        "变异区间偏移":XX, #变异区间距离M_before_muta头部的偏移，以字节为单位
        "变异区间长度":XX, }}, #即strlen(变异区间内容)，以字节为单位
        "M_after_muta变异区间定位信息":
        {{
        "报文位置索引":XX, #变异区间所处报文在M_after_muta列表中的位置索引
        "报文请求方法":"XXX", 
        "变异区间偏移":XX,
        "变异区间长度":XX, }},
        "结构敏感性分数": XX,   # 根据各变异区间的变异对协议状态转换的重要性进行评分，分数在5-15之间，分数越高，变异对状态转换的影响越大
        "评分依据/原因":"",
        # 分析该变异区间是否有不同于关键报文对的其他变异方法可以导致相同状态转换,给出变异后的报文片段，并给出变异后可能到达的状态（状态用数字表示）,例举出至少3种方法
        "导致相同状态转换的其他变异方法": [{{"变异序号":1, "变异后片段":"XXX", "可能转换到的状态":"XXX"}},
                                    {{"变异序号":2, "变异后片段":"XXX", "可能转换到的状态":"XXX"}},
                                    {{"变异序号":3, "变异后片段":"XXX", "可能转换到的状态":"XXX"}}]
        }},
        {{
        "变异区间序号": XX,
        #格式同上
        }}
        ```
        注意事项:
        1. 变异区间是报文对间不同点的最小单位,其长度只能小于或等于报文的最小单元（如请求报文的Method、URI、Version、CSeq、Content-Type、User-Agent、Content-Length、Entity Body、End-of-Header Marker等字段）,若变异区间长度大于报文最小单元,需要将其拆分为多个小变异区间分别进行分析
        2. 一个报文对若包含多个变异区间,需要拆分进行分别分析,每个变异区间用一个单独的变异区间序号进行标识
        3. 你必须完整地对所有关键报文对的所有变异区间进行分析
        4. 即使实际输出篇幅很长而超过单次回答字数上限,也不能省略任何分析结果
        5. 在分析完毕后,对所有变异区间的分析结果都要按给出json格式重新完整输出一遍
        """


        return prompt

    inserted_commas_q1 = insert_commas_and_quotes(q1)
    inserted_commas_q2 = insert_commas_and_quotes(q2)
    # Generate prompt
    prompt = generate_prompt(inserted_commas_q1, s1, inserted_commas_q2, s2)
    print("prompt:  ", prompt)


    # # Call GPT-4 with the generated prompt
    # response = openai.Completion.create(
    #     model="text-davinci-003",
    #     prompt=prompt,
    #     max_tokens=1500,
    #     temperature=0.7
    # )

    # # 解析响应
    # gpt_response = response["choices"][0]["text"].strip()

    # # Save the response to a JSON file
    # timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
    # filename = f'./cc-rule-base/answer/answer-{timestamp}.json'
    # with open(filename, 'w', encoding='utf-8') as file:
    #     json.dump(json.loads(gpt_response), file, ensure_ascii=False, indent=4)

    # print(f'Response saved to {filename}')

    # return gpt_response



def update_table_rule(q1, s1, q2, s2):
    global table_rule

    gpt_response = analyze_rtsp_mutation(q1, s1, q2, s2, common_parts, diff_parts_q1, diff_parts_q2)
    gpt_data = json.loads(gpt_response)

    for diff in gpt_data:
        pre_state = diff["变异前状态"]
        post_state = diff["变异后状态"]
        importance_score = diff["ImportanceScore"]
        q1_mutation_info = diff["q1变异区间位置信息"]
        other_mutation_methods = diff["OtherMutationMethods"]

        if post_state not in table_rule:
            table_rule[post_state] = omdict()

        entry = {
            "原状态": pre_state,
            "变异区间位置及相应变异方法": {
                "变异区间内容": q1_mutation_info["变异区间内容"],
                "报文请求方法": q1_mutation_info["报文请求方法"],
                "报文字段名": q1_mutation_info["报文字段名"],
                "报文字段偏移": q1_mutation_info["报文字段偏移"],
                "变异区间长度": q1_mutation_info["变异区间长度"],
                "其他变异方法": other_mutation_methods
            },
            "ImportanceScore": importance_score
        }

        table_rule[post_state].append(entry)

    # Save table_rule to a JSON file
    with open('table_rule.json', 'w', encoding='utf-8') as f:
        json.dump(table_rule, f, ensure_ascii=False, indent=4)

def Update_mutation_rule_table(q1, s1, q2, s2):
    
    update_table_rule(q1, s1, q2, s2)
    # Print the updated table_rule for verification
    print(json.dumps(table_rule, ensure_ascii=False, indent=4))



if __name__ == "__main__":
    q1 = "PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8552/w�vAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-AgentS ./testRTSPClient (LIVE555 Streaming Media v201Q.08.28)\r\nSesS~on: 000022B8\r\nRange: npt=0.000-\r\n\r\nSETUP rtsf://127.0.0.K:8554/wavAudioTest/RangeP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTranspor:aming Media v201 R\r\nRanPLAYnpt=0.000-\r\n\r\nSETUP TP/AVP;unicas "
    q2 = "PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nDESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: .!tPAUSEgent: .Dtest@TSPClient (LIVE555 Streaming Media v�"
    s1 = [454, 454, 400, 400]
    s2 = [454, 200, 404]
    # common_parts, diff_parts_q1, diff_parts_q2 = compare_rtsp_messages(q1, q2)
    analyze_rtsp_mutation(q1, s1, q2, s2)



    


    