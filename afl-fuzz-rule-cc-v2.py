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


def compare_rtsp_messages(q1, q2):
    import difflib

    # Function to find common parts
    def find_common_parts(seq1, seq2):
        matcher = difflib.SequenceMatcher(None, seq1, seq2)
        common_parts = []
        for tag, i1, i2, j1, j2 in matcher.get_opcodes():
            if tag == 'equal':
                common_parts.append(seq1[i1:i2])
        return ''.join(common_parts)

    # Function to find different parts
    def find_diff_parts(seq1, seq2):
        matcher = difflib.SequenceMatcher(None, seq1, seq2)
        diff_parts_seq1 = []
        diff_parts_seq2 = []
        for tag, i1, i2, j1, j2 in matcher.get_opcodes():
            if tag != 'equal':
                diff_parts_seq1.append(seq1[i1:i2])
                diff_parts_seq2.append(seq2[j1:j2])
        return ''.join(diff_parts_seq1), ''.join(diff_parts_seq2)

    # Get common and different parts
    common_parts = find_common_parts(q1, q2)
    diff_parts_q1, diff_parts_q2 = find_diff_parts(q1, q2)

    return common_parts, diff_parts_q1, diff_parts_q2

# def process_message_sequences(q1, s1, q2, s2):
#     # 计算报文数
#     messages_q1 = q1.split("\r\n\r\n")
#     messages_q2 = q2.split("\r\n\r\n")
    
#     n_q1 = len(messages_q1) 
#     n_q2 = len(messages_q2)
#     # print("报文数:", n_q1, n_q2)
    
#     # 遍历s1和s2，找到不同的位置
#     pos_diff_status = []
#     json_diff_queue_status = []
    
#     for i in range(min(len(s1), len(s2), n_q1, n_q2)):
#         # print("---------------------")
#         if s1[i] != s2[i]:
#             pos_diff_status.append(i)
#             # print("---------------------")
#             json_diff_queue_status.append({
#                 "报文对位置": i-1,
#                 "q1": {
#                     "变异前报文": messages_q1[i-1],
#                     "变异前状态": s1[i]
#                 },
#                 "q2": {
#                     "变异后报文": messages_q2[i-1],
#                     "变异后状态": s2[i]
#                 }
#             })
    
#     return pos_diff_status, json_diff_queue_status

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
    
    # 遍历两个列表的前min_length个元素，比较它们
    for i in range(min_length):
        if s1[i] != s2[i]:
            differences.append(i)
    
    return differences


def analyze_rtsp_mutation(q1, s1, q2, s2, common_parts, diff_parts_q1, diff_parts_q2):
    # Function to generate prompt based on input
    def generate_prompt(q1, s1, q2, s2, common_parts, diff_parts_q1, diff_parts_q2):
        # prompt = f"""
        # q1:

        # ```
        # {{
        #     "m1":"{common_parts}",
        #     "m2":"{diff_parts_q1}"
        # }}
        # ```

        # q2:

        # ```
        # {{
        #     "m1":"{common_parts}",
        #     "m2":"{diff_parts_q2}"
        # }}
        # ```

        # s1:{s1}

        # s2:{s2}

        # 我正在对RTSP协议进行协议模糊测试。q1与q2为报文序列，报文序列由m1和m2组成，m1部分为q1与q2相同的部分，且始终在报文序列的前段，m2部分为q1与q2不同的部分，在报文序列后段。通过对q1的m2部分进行变异得到q2。s1为q1到达的状态序列，s2为q2到达的状态序列，请重点关注q1、q2的m2部分及相关状态，按以下步骤分析m2部分的变异对状态转换的影响，并给出变异方法指导：

        # 1. 找出m2部分的变异位段，各变异位段通过其与报文字段的相对位置来定位（偏移、长度）
        # 2. 请你对这些变异位段进行分析，筛选出有可能导致状态序列由s1转换到s2的变异位段，分别对这些变异位段的状态转换重要性进行评分
        # 3. 这些变异位段是否有其他变异方法可以导致相同的状态转换，给出变异后的该片段
        # 4. 运用你已有的触发协议漏洞相关知识，给出这些变异位段可以如何变异以导致协议崩溃，给出变异后的该片段

        # 注意：

        # 1. 变异位段是指两报文不同部分的最小化具体位置，而不是不同部分所在的整个字段（这样范围太大，我需要精确到最小变异位段）
        # 2. 报文序列包含若干个报文，且报文有可能不为完整的格式，请以分隔符”\r\n\r\n“区分前后不同报文

        # 你的回答需要以json为输出格式，格式如下：

        # ```
        # {{
        # # 描述变异类型
        # "Difference1": "XXX",
        # "变异类型":"XXX",
        # "变异前状态":XXX,
        # "变异后状态":XXX,
        # # 描述变异位段位置
        # "q1变异位段位置信息":  
        # {{"变异位段内容":"XXX", 
        # "报文请求方法":"XXX", #变异位段所在报文的请求方法
        # "报文字段名":"XXX", #变异位段所在字段名，使用该协议标准字段名
        # "报文字段偏移":XX, #变异位段在所在字段的偏移
        # "变异位段长度":XX, }},
        # "q2变异位段位置信息":
        # {{"变异位段内容":"XXX",
        # "报文请求方法":"XXX", 
        # "报文字段":"XXX",
        # "报文字段偏移":XX,
        # "变异位段长度":XX, }},
        # # 评价该变异位段对状态转换重要性，若变异该片段更可能导致状态转换则给出更高分数，并给出打分的依据/原因
        # "ImportanceScore": XX,
        # "打分依据/原因":"",
        # # 分析推断该片段还可以如何变异能导致协议状态转换，直接给出变异后的该片段和变异后可能到达的状态（状态用数字表示），可以有多个
        # "OtherMutationMethods": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
        #                             {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
        #                             {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}],
        # "可能导致崩溃的变异方法": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX","为什么会导致奔溃":"XXX"}},
        #                             {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX","为什么会导致奔溃":"XXX"}},
        #                             {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}]
        # }},
        # {{
        # "Difference2": "XXX",
        # #格式同上
        # }}
        # ```


        # """

        # prompt = rf"""
        # q1:[{q1}]

        # s1:{s1}

        # q2:[{q2}]

        # s2:{s2}

        # 我正在对RTSP协议进行协议模糊测试。q1与q2为报文序列，q2由q1经过部分变异得到。s1为q1到达的状态序列，s2为q2到达的状态序列，请重点关注q1到q2的变异部分所在报文及相关状态，按以下步骤进行分析和回答：

        # 1. 为q1和q2的各报文标上序号
        # 2. 找出q1与q2的不同位段（即q1到q2的变异位段）
        # 3. 对这些变异位段进行分析，筛选出有可能导致状态序列由s1转换到s2的变异位段，分别对这些变异位段的状态转换重要性进行评分
        # 4. 这些变异位段是否有其他变异方法可以导致相同的状态转换，给出变异后的该片段
        # 5. 运用你已有的触发协议漏洞相关知识，给出这些变异位段可以如何变异以导致协议崩溃，给出变异后的该片段

        # 注意：

        # 1. 变异位段是指两报文不同部分的最小化具体位置，而不是不同部分所在的整个字段（这样范围太大，我需要精确到最小变异位段）
        # 2. 报文序列包含若干个报文，且报文有可能不为完整的格式

        # 你的回答只能以json为输出格式，且不要包含中间分析步骤，输出格式如下：

        # ```
        # {{
        # # 描述变异类型
        # "Difference1": "XXX",
        # "变异类型":"XXX",
        # "变异前状态":XXX, #变异位段所在报文触发状态
        # "变异后状态":XXX,
        # # 描述变异位段位置
        # "q1变异位段位置信息":  
        # {{"变异位段内容":"XXX", 
        # "报文序号":XX, #变异位段所处报文在q1序列中的序号
        # "报文请求方法":"XXX", #变异位段所在报文的请求方法
        # "报文字段名":"XXX", #变异位段所在字段名，使用该协议标准字段名
        # "报文字段偏移":XX, #变异位段在所在字段的偏移
        # "变异位段长度":XX, }},
        # "q2变异位段位置信息":
        # {{"变异位段内容":"XXX",
        # "报文序号":XX, #变异位段所处报文在q2序列中的序号
        # "报文请求方法":"XXX", 
        # "报文字段":"XXX",
        # "报文字段偏移":XX,
        # "变异位段长度":XX, }},
        # # 评价该变异位段对状态转换重要性，若变异该片段更可能导致状态转换则给出更高分数，并给出打分的依据/原因
        # "ImportanceScore": XX,
        # "打分依据/原因":"",
        # # 分析推断该片段还可以如何变异能导致协议状态转换，直接给出变异后的该片段和变异后可能到达的状态（状态用数字表示），可以有多个
        # "OtherMutationMethods": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
        #                             {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
        #                             {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}],
        # "可能导致崩溃的变异方法": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX","为什么会导致奔溃":"XXX"}},
        #                             {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX","为什么会导致奔溃":"XXX"}},
        #                             {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}]
        # }},
        # {{
        # "Difference2": "XXX",
        # #格式同上
        # }}
        # ```
        # """

        # ，分别对这些变异位段的状态转换重要性进行评分
        # 4. 这些变异位段是否有其他变异方法可以导致相同的状态转换，给出变异后的该片段
        # 5. 运用你已有的触发协议漏洞相关知识，给出这些变异位段可以如何变异以导致协议崩溃，给出变异后的该片段

        # 注意：

        # 1. 变异位段是指两报文不同部分的最小化具体位置，而不是不同部分所在的整个字段（这样范围太大，我需要精确到最小变异位段）
        # 2. 报文序列包含若干个报文，且报文有可能不为完整的格式
        # 重点关注q1到q2的变异部分所在报文及相关状态，

        # prompt = rf"""
        # q1:[{q1}]

        # s1:{s1}

        # q2:[{q2}]

        # s2:{s2}

        # 我正在对RTSP协议进行协议模糊测试。q1与q2为报文序列，q2由q1经过部分变异得到。s1为q1到达的状态序列，s2为q2到达的状态序列，请按以下步骤进行分析：
        # 1. 找出导致状态序列变化的报文对
        # 2. 对这些报文对分别进行分析，找出所有从q1报文到q2报文的变异位段
        # 3. 列出所有可能导致状态转换的变异位段并进行定位，这些变异位段通过其与所在报文字段的相对位置来定位（偏移、长度）

        # 注意：
        # 1. 变异位段是指两报文不同部分的最小化具体位置，而不是不同部分所在的整个字段（这样范围太大，我需要精确到最小变异位段），若两报文有多个变异位段，需要分别进行分析
        # 2. 报文序列包含若干个报文，且报文有可能不为完整的格式

        # 你可以按以上步骤分布思考，但中间的思考不要显示在回答中，给出的回答只能以如下格式输出：

        # ```
        # {{
        # # 描述变异类型
        # "Difference1": "XXX",
        # "变异类型":"XXX",
        # "变异前状态":XXX, #变异位段所在报文触发状态
        # "变异后状态":XXX,
        # # 描述变异位段位置
        # "q1变异位段内容":"XXX", 
        # "q2变异位段内容":"XXX",
        # "q1变异位段定位信息":  
        # {{
        # "报文序号":XX, #变异位段所处报文在q1序列中的序号
        # "报文请求方法":"XXX", #变异位段所在报文的请求方法
        # "报文字段名":"XXX", #变异位段所在字段名，使用该协议标准字段名
        # "报文字段偏移":XX, #变异位段在所在字段的偏移
        # "变异位段长度":XX, }},
        # "q2变异位段定位信息":
        # {{
        # "报文序号":XX, #变异位段所处报文在q2序列中的序号
        # "报文请求方法":"XXX", 
        # "报文字段":"XXX",
        # "报文字段偏移":XX,
        # "变异位段长度":XX, }},
        # }},
        # {{
        # "Difference2": "XXX",
        # #格式同上
        # }}
        # ```
        # """

        diff_pos = find_differences(s1, s2)
        result_string = ""
        for i in diff_pos:
            if result_string:  # 如果result_string不为空，加入逗号和空格进行分隔
                result_string += "、"
            # 根据索引从q1和q2中取值，格式化字符串
            result_string += f"M_before_muta[{i}]到M_after_muta[{i}]"
        prompt = rf"""
        M_before_muta:[{q1}]

        S_before_muta:{s1}

        M_after_muta:[{q2}]

        S_after_muta:{s2}

        我正在对RTSP协议进行协议模糊测试。M_before_muta、M_before_muta、S_before_muta、S_after_muta均为列表，M_before_muta与M_before_muta为报文列表，M_before_muta由M_before_muta经过部分变异得到。S_before_muta为M_before_muta到达的状态列表，两者数量一一对应。S_after_muta为M_before_muta到达的状态列表，两者数量一一对应。
        将S_before_muta与S_after_muta进行比对后，发现S_before_muta[1]与S_after_muta[1]、S_before_muta[2]与S_after_muta[2]不同，由此推断出{result_string}的变异产生了状态转换，请关注这2组产生状态转换的关键报文对并按以下步骤进行分析：
        1. 找出每个报文对的所有变异位段，若1个报文对包含多个变异位段，需要赋予不同变异序号并分别进行分析
        2. 列出所有可能导致状态转换的变异位段并进行定位，这些变异位段通过其与所在报文字段的相对位置来定位（偏移、长度）
        3. 根据各变异位段的变异对协议状态转换的重要性，分别对这些变异位段的"结构敏感性分数"进行评分，并给出打分的依据/原因
        4. 分析这些变异位段是否有不同于关键报文对的其他变异方法可以导致相同状态转换，给出变异后的该片段
    
        注意：
        1. 变异位段是报文对间不同点的最小单位，其长度只能小于或等于报文的最小单元（RTSP报文最小单元按顺序依次为Method、URI、Version、CSeq、Content-Type、User-Agent、Content-Length、Entity Body、End-of-Header Marker），若变异位段长度大于最小单元，需要将其拆分为多个变异位段分别进行分析
        2. 一个报文对若包含多个变异位段，需要拆分进行分别分析，每个变异位段用一个单独的变异位段序号进行标识
        3. 一个报文列表中可能包含若干个报文，且报文有可能为非完整的格式
        4. 你必须完整的对所有关键报文对的所有变异位段进行分析，不同的关键报文对的分析结果之间不要分段输出，必须将所有的变异位段的分析结果以要求的json格式一次性完整输出
    
        即使实际输出篇幅很长超过单次回答字数上限，也不能省略任何分析结果。你的分析结果必须按如下json格式完整输出：
    
        ```
        {{
        "变异位段序号": XX,
        "所述报文对": "M_before_muta[i]到M_before_muta[i]", 
        "变异类型":"XXX",
        "变异前状态":S_before_muta[i], #变异位段所在报文触发状态
        "变异后状态":S_after_muta[i],
        # 描述变异位段位置
        "M_before_muta变异位段内容":"XXX", 
        "M_before_muta变异位段内容":"XXX",
        "M_before_muta变异位段定位信息":  
        {{
        "报文位置索引":XX, #变异位段所处报文在M_before_muta列表中的位置索引
        "报文请求方法":"XXX", #变异位段所在报文的请求方法
        "报文字段名":"XXX", #变异位段所在字段名，使用该协议标准字段名
        "报文字段偏移":XX, #变异位段在所在字段的偏移，纯数字
        "变异位段长度":XX, }}, #变异位段的长度，纯数字
        "M_before_muta变异位段定位信息":
        {{
        "报文位置索引":XX, #变异位段所处报文在M_before_muta列表中的位置索引
        "报文请求方法":"XXX", 
        "报文字段":"XXX",
        "报文字段偏移":XX,
        "变异位段长度":XX, }},
        "结构敏感性分数": XX,
        "打分依据/原因":"",
        # 分析推断该片段还可以如何变异能导致协议状态转换，直接给出变异后的该片段和变异后可能到达的状态（状态用数字表示），可以有多个
        "导致相同状态转换的其他变异方法": [{{"变异1":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
                                    {{"变异2":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}},
                                    {{"变异3":"XXX","变异依据/原因":"XXX","可能转换到的状态":"XXX","为什么转换到该状态":"XXX"}}]
        }},
        {{
        "变异位段序号": XX,
        #格式同上
        }}
        ```
        """


        return prompt

    inserted_commas_q1 = insert_commas_and_quotes(q1)
    inserted_commas_q2 = insert_commas_and_quotes(q2)
    # Generate prompt
    prompt = generate_prompt(inserted_commas_q1, s1, inserted_commas_q2, s2, common_parts, diff_parts_q1, diff_parts_q2)
    print("prompt:  ", prompt)


    # Call GPT-4 with the generated prompt
    response = openai.Completion.create(
        model="text-davinci-003",
        prompt=prompt,
        max_tokens=1500,
        temperature=0.7
    )

    # 解析响应
    gpt_response = response["choices"][0]["text"].strip()

    # Save the response to a JSON file
    timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
    filename = f'./cc-rule-base/answer/answer-{timestamp}.json'
    with open(filename, 'w', encoding='utf-8') as file:
        json.dump(json.loads(gpt_response), file, ensure_ascii=False, indent=4)

    print(f'Response saved to {filename}')

    return gpt_response



def update_table_rule(q1, s1, q2, s2, key_variant_message):
    global table_rule

    gpt_response = analyze_rtsp_mutation(q1, s1, q2, s2, common_parts, diff_parts_q1, diff_parts_q2)
    gpt_data = json.loads(gpt_response)

    for diff in gpt_data:
        pre_state = diff["变异前状态"]
        post_state = diff["变异后状态"]
        importance_score = diff["ImportanceScore"]
        q1_mutation_info = diff["q1变异位段位置信息"]
        other_mutation_methods = diff["OtherMutationMethods"]

        if post_state not in table_rule:
            table_rule[post_state] = omdict()

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

        table_rule[post_state].append(entry)

    # Save table_rule to a JSON file
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

    # # 示例数据列表
    # mutation_data = [
    #     {
    #         "kl_messages": "TP/\u0013.q\u001a 5\r\nSETUPUser-Agent: ./testRTSPCdieAudioTuqt\u000f RMSP/1.0\r\n_Seq: 5\r\nU\u0001",
    #         "state_sequence": [0, 400, 400, 400, 200, 400, 400, 405, 200, 405, 405, 454, 405, 400, 405, 405, 200, 405, 405, 454, 454, 405, 405, 400, 405, 400, 405, 400, 400, 405, 200, 405, 405, 454, 405, 400, 405, 405, 200, 405, 405, 454, 454, 405, 405, 400, 405, 400, 405, 400],
    #         "state_count": 50
    #     },
    #     {
    #         "kl_messages": "DESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3(\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unic`st;client_port=37952-37953\r\n#\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE55\u0013 Streaming Media v2018.08.28)\r\nTranspnit: RTP/AVP;unicast;client_port=3795tsp://12nge: npt=0.000-\r\n\r\nDst RTQP/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./testRTSPClient (LIVE555 Streaming Media v20?\u0018.08'28)\r\nAccept: a�Transport/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGeT_PARAMETERtestRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSess�on: 000022B8\r\nRange6 npt=0.000-\r\n\r\nDESCRIBE rtsp://1\u001d7.0H0.1:85u4/wavioTest RTQP/1.0\r\nCSeq: 2\r\nUs�CSeq: 2\r\nUstestRTSPClSET_PARAMETERient (LIVE555 Streaming Media v20?P/1.0\r\nCSeq: 2\r\nUs�r-Agent: ./tatimn/sdp\r\n\r\nPLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUGET_PARAMETERtestRTSPClient (LIVE555 Streaming55 St Media v2018.08.28)\r\nSe�sion: 000022B8\r�Range:  rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streami08.28)\r\nTTP/AVP;unicast;client_port=37952-37953\r\n\r\nTEARDOWN rtsp://127.0.0.1:8554/wavAudioTest/ RTSs/1.0\r\nCSeq: 5\r\n\\ser-Agent: ./tesOPTIO\u0010StRTSPClient (LIVE555 Streaming Media v2018.08�",
    #         "state_sequence": [0, 400, 454, 454, 200, 400, 454, 400, 454, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 400, 400, 400, 200, 404, 405, 400, 400, 400, 200, 400, 200, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200, 400, 400, 200, 400, 200],
    #         "state_count": 135
    #     },
    #     # 添加更多的变异数据...
    # ]
    # for data in mutation_data:
    #     q1 = data["kl_messages"]
    #     s1 = data["state_sequence"]
    #     q2 = data["kl_messages"]  # 使用变异后的消息作为q2，s2
    #     s2 = data["state_sequence"]

    #     Update_mutation_rule_table(q1, s1, q2, s2)

    q1 = "PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8552/w�vAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-AgentS ./testRTSPClient (LIVE555 Streaming Media v201Q.08.28)\r\nSesS~on: 000022B8\r\nRange: npt=0.000-\r\n\r\nSETUP rtsf://127.0.0.K:8554/wavAudioTest/RangeP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTranspor:aming Media v201 R\r\nRanPLAYnpt=0.000-\r\n\r\nSETUP TP/AVP;unicas "
    q2 = "PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nDESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: .!tPAUSEgent: .Dtest@TSPClient (LIVE555 Streaming Media v�"
    s1 = [454, 454, 400, 400]
    s2 = [454, 200, 404]
    # inserted_commas_q1 = insert_commas_and_quotes(q1)
    # print(inserted_commas_q1)   
    common_parts, diff_parts_q1, diff_parts_q2 = compare_rtsp_messages(q1, q2)
    analyze_rtsp_mutation(q1, s1, q2, s2, common_parts, diff_parts_q1, diff_parts_q2)

    # print("Common parts:", common_parts)
    # print("Different parts in q1:", diff_parts_q1)
    # print("Different parts in q2:", diff_parts_q2)


    


    