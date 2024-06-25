#include <stdio.h>
#include <string.h>

#define MAX_STATUS 200

int* compare_protocols(const char* q1, const int* s1, const char* q2, const int* s2, int* pos_diff_status, int* diff_count) {
    // 计算q1, q2的总报文数
    int n_q1 = 0, n_q2 = 0;
    const char* ptr = q1;
    while ((ptr = strstr(ptr, "\r\n\r\n")) != NULL) {
        n_q1++;
        ptr += 4;
    }
    ptr = q2;
    while ((ptr = strstr(ptr, "\r\n\r\n")) != NULL) {
        n_q2++;
        ptr += 4;
    }

    // 同时遍历s1, s2，找出不同的位置
    int i;
    for (i = 0; i < MAX_STATUS && i < n_q1 && i < n_q2; i++) {
        if (s1[i] != s2[i]) {
            pos_diff_status[(*diff_count)++] = i;
        }
    }

    return pos_diff_status;
}

int main() {
    const char* q1 = "DESCRIBE rtsp://127.0.0\u001d1:8554/wavAudioTest RTSP/1.0\r\nCSeq:\t2\r\nrser-Agent: ./tes[RTSPClient (LIVE555 Streeaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTEARDOWN rtsO://127.0.0.1:8554/wavAudioTest/(RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: .PAUSERTSPClientGET_PARAMETER (LIVE555 Sdreamitg Media v2018.08.28)\r\nSessi�n: ";
    int s1[] = {200, 454, 454, 400, 200, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 454, 404, 405, 400, 400, 454, 200, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 400, 454, 404, 400, 400, 400, 454, 404, 405, 400, 400, 454, 404, 405, 400, 405, 404, 405, 400, 400, 454, 454, 200, 200, 200, 454, 454, 405, 400, 405, 454, 200, 405, 200, 200, 404, 404, 400, 200};
    const char* q2 = "DESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nAccept= application v20Q8.0DES rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: ./testRTAgent: ./testRTSPClient (LIVE555 Etreaming Media v2018.08.28)\r\nSession: 000022B8\r\n\r\nTE,RDOSETUPWN rtsPAUS<pR//127.0.0.1e8d54/wavAudgPLAYoTest/ RTSP/1�0\r�CSeq: \r\nUng PTIONSiov2018.08.28)\r\nAccept:\u000f0NSiov2018K22B8\r\n\r\nTEARDOWN rtsp://127.0.CCCCCCCCCCCCCCCCCCCCCCCCTSP/1.0\r\nCSeq: 5\r\nUser-AgeLt: ./tesZRTS�Client (LIVE555 Samingac3OudioTest.08.28)\r\nSession: 000022;8\r\n\r\nDESCRIBE rtsp:/�\u000327�.0.1:8554/wavAud1:8554/wavA8554/wavAudioioTest RCSe";
    int s2[] = {200, 405, 400, 400, 454, 200, 200, 200, 400, 454, 200, 405, 200, 200, 404, 404};

    int pos_diff_status[MAX_STATUS] = {0};
    int diff_count = 0;

    compare_protocols(q1, s1, q2, s2, pos_diff_status, &diff_count);

    printf("Differences at positions:\n");
    for (int i = 0; i < diff_count; i++) {
        printf("%d ", pos_diff_status[i]);
    }
    printf("\n");

    return 0;
}
