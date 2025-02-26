#include <absacc.h>     // ��չ�ӿڿ�
#include <intrins.h>    // ��λ������
#include <reg52.h>      // ����89C52��

// �˴��л�����������Ϳ��������
#if 1
#define DISP_SEG P0 //��չ�ӿ�8255(U3)PA�ڵ�ַ,������ֶ������
#define DISP_BIT P2 //��չ�ӿ�8255(U3)PB�ڵ�ַ,�������λ�����
#define VKEY_RL P1  //��չ�ӿ�8255(U3)PC�ڵ�ַ,��������С��нӿ�
#else
#define DISP_SEG XBYTE[0xfefc]   //��չ�ӿ�8255(U3)PA�ڵ�ַ,������ֶ������
#define DISP_BIT XBYTE[0xfefd]   //��չ�ӿ�8255(U3)PB�ڵ�ַ,�������λ�����
#define VKEY_RL XBYTE[0xfefe]    //��չ�ӿ�8255(U3)PC�ڵ�ַ,��������С��нӿ�
#define U3CON XBYTE[0xfeff]      //��չ�ӿ�8255(U3)���ƿڵ�ַ
#endif
unsigned char gearTime = 0;         // ����ʱ�䣨�룩
sbit gear = P3 ^ 0;
sbit beep = P3 ^ 2;
// sbit key6 = P3^2;

// ��ʾ����
unsigned char dispBuf[10] = {0x10};
enum { MENU_TIME, MENU_DATE, MENU_PWD, MENU_PWD_CHANGE1, MENU_PWD_CHANGE2, MENU_PWD_CHANGE3, MENU_ZIJIAN };

// �ַ��ֵ�
enum {
    DK_0,
    DK_1,
    DK_2,
    DK_3,
    DK_4,
    DK_5,
    DK_6,
    DK_7,
    DK_8,
    DK_9,
    DK_A,
    DK_B,
    DK_C,
    DK_D,
    DK_E,
    DK_F,
    DK_Space,
    DK_Del,
    DK_UdLine,
    DK_Dot,
    DK_D0,
    DK_D1,
    DK_D2,
    DK_D3,
    DK_D4,
    DK_D5,
    DK_D6,
    DK_D7,
    DK_D8,
    DK_D9,
    DK_DA,
    DK_DB,
    DK_DC,
    DK_DD,
    DK_DE,
    DK_DF,
    DK_NUL,
    DK_EOF,
    DK_P,
    DK_Equal,
    DK_Rectangle
};
// �ַ��ֶ�
unsigned char code segtable[] = {
    0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, // 0~9�ֶ���
    0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e, 0xff, 0xbf, 0xf7, 0x7f, // A~F,�ո�,-,_,.�ֶ���
    0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x10, // 0.~9.�ֶ���
    0x08, 0x03, 0x46, 0x21, 0x06, 0x0e,                         // A.~F.�ֶ���
    0xcc, 0xee, 0x8c, 0xb7, 0xa3                                // ����,������,P,=,������
};
unsigned char Scan_code[] = {0xee, 0xed, 0xeb, 0xe7, 0xde, 0xdd, 0xdb, 0xd7,
                             0xbe, 0xbd, 0xbb, 0xb7, 0x7e, 0x7d, 0x7b, 0x77}; //����ɨ����
unsigned char scan_buf[6] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10}; //���̹����˲����嵥Ԫ

// ����
unsigned char pwd[6] = {5, 3, 5, 8, 6, 6};  // Ĭ������
unsigned char input[6], pwdBack[6];         // �������롢������
unsigned char inputLen = 0;                 // ��������ĳ���
unsigned char triedTimes = 0;               // ����������������Ĵ���
unsigned char triedTimer = 0;               // �������������
unsigned int temp;                          // ��ʱ����

// �˵�
unsigned char menu = MENU_TIME;     // �˵�״̬����
unsigned char zijianpos = 0;        // �Լ���ʾλ�ñ���

unsigned char settime = 0;          // ����ʱ�� ��0�������ã�1������ʱ�䣬2����������
unsigned char sgClick = 0;          // �Ƿ񵥻�
unsigned char hFast = 0;            // ʱ�������
unsigned long hhFastTime = 0;       // ʱ�������
unsigned int blinkTime = 0;         // ���󰴼�����ʱ��Ĺ����˸��ʱ��
unsigned char blinkIndex = 0;       // ���󰴼�����ʱ��Ĺ����˸λ��
unsigned char playType = 0;         // �������ͣ�0�����ţ�1��������2ʧ�ܣ�3�ɹ�
unsigned int playTimer = 0;         // ��������һ����ʱ��
unsigned int playTime = 0;          // ���Ŵ���
unsigned char playSound = 0;        // ���ſ���

// ʱ������
unsigned int year = 2022;
unsigned char month = 1;
unsigned char day = 1;
unsigned char sec = 0;
unsigned char min = 0;
unsigned char hour = 0;
unsigned int tempA, tempB, tempC;   // ������ʱ�洢ʱ������

// ��ʱ���������뵥λ
void delay_ms(unsigned int j) {
    unsigned int k;
    for (; j > 0; j--)
        for (k = 0; k < 100; k++)
            ;
}

// ���֣���ÿ���˵��ֱ������ʾ����
void display_buf() {
    int i;
    switch (menu) {
    case MENU_TIME:
        if(settime == 1) {
            // ʱ
            dispBuf[0] = tempA / 10;
            dispBuf[1] = tempA % 10;
            dispBuf[2] = DK_Del;
            // ��
            dispBuf[3] = tempB / 10;
            dispBuf[4] = tempB % 10;
            dispBuf[5] = DK_Del;
            // ��
            dispBuf[6] = tempC / 10;
            dispBuf[7] = tempC % 10;
            dispBuf[8] = DK_EOF;
        } else {
            // ʱ
            dispBuf[0] = hour / 10;
            dispBuf[1] = hour % 10;
            dispBuf[2] = DK_Del;
            // ��
            dispBuf[3] = min / 10;
            dispBuf[4] = min % 10;
            dispBuf[5] = DK_Del;
            // ��
            dispBuf[6] = sec / 10;
            dispBuf[7] = sec % 10;
            dispBuf[8] = DK_EOF;
        }
        break;
    case MENU_DATE:
        if(settime == 2) {
            // ��
            dispBuf[0] = tempA / 1000;
            dispBuf[1] = (tempA - dispBuf[0] * 1000) / 100;
            dispBuf[2] = (tempA - tempA / 100 * 100) / 10;
            dispBuf[3] = DK_D0 + tempA % 10;
            // ��
            dispBuf[4] = tempB / 10;
            dispBuf[5] = DK_D0 + tempB % 10;
            // ��
            dispBuf[6] = tempC / 10;
            dispBuf[7] = tempC % 10;
            dispBuf[8] = DK_EOF;
        } else {
            // ��
            dispBuf[0] = year / 1000;
            dispBuf[1] = (year - dispBuf[0] * 1000) / 100;
            dispBuf[2] = (year - year / 100 * 100) / 10;
            dispBuf[3] = DK_D0 + year % 10;
            // ��
            dispBuf[4] = month / 10;
            dispBuf[5] = DK_D0 + month % 10;
            // ��
            dispBuf[6] = day / 10;
            dispBuf[7] = day % 10;
            dispBuf[8] = DK_EOF;
        }
        break;

    case MENU_PWD: case MENU_PWD_CHANGE1: case MENU_PWD_CHANGE2: case MENU_PWD_CHANGE3:
        if(triedTimes >= 3) {
            dispBuf[0] = DK_F;
            dispBuf[1] = DK_P;
            dispBuf[2] = DK_DD;
            dispBuf[3] = DK_Equal;
            dispBuf[4] = triedTimer / 10;
            dispBuf[5] = triedTimer % 10;
            dispBuf[6] = DK_D5;
            dispBuf[7] = DK_Space;
        } else {
            if(menu == MENU_PWD || menu == MENU_PWD_CHANGE1) {
                dispBuf[0] = DK_P;
                dispBuf[1] = DK_Equal;
            } else if(menu == MENU_PWD_CHANGE2) {
                dispBuf[0] = DK_P;
                dispBuf[1] = DK_D1;
            } else if(menu == MENU_PWD_CHANGE3) {
                dispBuf[0] = DK_P;
                dispBuf[1] = DK_D2;
            }
            for (i = 2; i < 8; i++) {
                if (i - 2 >= inputLen)
                   break;
                if (menu == MENU_PWD_CHANGE2 || menu == MENU_PWD_CHANGE3)
                    dispBuf[i] = input[i - 2];
                else
                    dispBuf[i] = DK_Rectangle;
            }
            for (; i < 8; i++)
                dispBuf[i] = DK_UdLine;
            dispBuf[8] = DK_NUL;
        }
        break;
    case MENU_ZIJIAN:
        for (i=0; i<8; i++) {
            if (i == zijianpos) {
                dispBuf[i] = DK_Del;
            } else dispBuf[i] = DK_Space;
        }
        break;
    }
}

// ��ʾ���浽�����
void display_swbuf() {
    unsigned int i, j;
#ifdef U3CON
    unsigned int disp_bit_con = 0x80; //��λ�������ֵ
#else
    unsigned int disp_bit_con = 0x01; //��λ�������ֵ
#endif
    DISP_SEG = 0xff;                  //����ʾ��ʹ��λ����һ�¡�����Ƶ��

    for (i = 0, j = 0; i < 10 && dispBuf[i] != DK_EOF; i++) {
        // ��˸λ
        if (!(settime && blinkIndex == i && blinkTime < 900)) {
            if (dispBuf[i] == DK_NUL) // �������
                continue;
            DISP_BIT = disp_bit_con;         // 8255��U3����B�������λ������
            DISP_SEG = segtable[dispBuf[i]]; //�Ի����ݲ��ֶ����õ��ֶ���;
            delay_ms(1);                     //��ʱ1ms
            DISP_SEG = 0xff;                 //����ʾ��ʹ��λ����һ�¡�����Ƶ��
        }
#ifdef U3CON
        disp_bit_con >>= 1; //��λ����������һλ
#else
        disp_bit_con <<= 1; //��λ����������һλ
#endif
        j++;
    }
    for (; i < 10; i++) {
        DISP_BIT = disp_bit_con;       // 8255��U3����B�������λ������
        DISP_SEG = segtable[DK_Space]; //�Ի����ݲ��ֶ����õ��ֶ���;
        delay_ms(1);                   //��ʱ1ms
        DISP_SEG = 0xff;               //����ʾ��ʹ��λ����һ�¡�����Ƶ��
#ifdef U3CON
        disp_bit_con >>= 1;            //��λ����������һλ
#else
        disp_bit_con <<= 1;            //��λ����������һλ
#endif
    }
}

// ��ʱ����ʼ��
void timer_init() {
    TMOD = 0x22; // T1 ��ʽ2����ʱ��GATE=0��T0 ��ʽ2����ʱ��GATE=0
    TH1 = 0x00;
    TL1 = 0x00; //��T1������ֵ0,����256��ÿ���ж϶�ʱ256x12/11.0592 us
    TH0 = 0x00;
    TL0 = 0x00; //��T1������ֵ0,����256��ÿ���ж϶�ʱ256x12/11.0592 us
    EA = 1;     //���ж�������
    //ET1 = 1;    // T1�ж�����
    //TR1 = 1;    //����T1��ʱ(ʱ���Ƿ���ʱ���� 1/0)������
    ET0 = 1;    // T1�ж�����
    TR0 = 1;    //����T1��ʱ(ʱ���Ƿ���ʱ���� 1/0)������
}



// �ж��Ƿ�������
unsigned char isLeapYear(unsigned int y) {
    return y%400 == 0 || y % 4 == 0 && y %100 != 0;
}

// ��ȡĳ���µ�����
unsigned char getDayOfMonth(unsigned int y, unsigned char m) {
    switch(m) {
        case 1:case 3:case 5:case 7:case 8:case 10:case 12:
            return 31;
        case 2: return isLeapYear(y) ? 29:28;
        default:
            return 30;
    }
}

// ����ʱ�䴦����
void updateDateTime(unsigned char set, unsigned char index)
{
    switch(set) {
        case 0:
        if (sec >= 60) {
            sec = 0;
            min++;
            if (min >= 60) {
                min = 0;
                hour++;
                if (hour >= 24) {
                    hour = 0;
                    day++;
                    if(day >= getDayOfMonth(year, month) + 1) {
                        day = 1;
                        month++;
                        if(month >= 13) {
                            month = 1;
                            year++;
                            if(year >= 9999) {
                                year = 2000;
                            }
                        }
                    }
                }
            }
        }
        break;
        
        case 1:
            switch(index) {
                case 7: case 6:
                    if (tempC >= 60) {
                        tempC = 0;
                        tempB++;
                    }
                case 4:case 3:
                    if (tempB >= 60) {
                        tempB = 0;
                        tempA++;
                    }
                case 1:case 0:
                    if (tempA >= 24) {
                        tempA = 0;
                    }
            }
        break;
        
        case 2:
            switch(index) {
                case 7:case 6:case 5:case 4:
                    if (tempC >= getDayOfMonth(tempA, tempB) + 1) {
                        tempC = 1;
                        tempB++;
                    }
                case 3:case 2:
                    if (tempB >= 13) {
                        tempB = 1;
                        tempA++;
                    }
                case 1:case 0:
                    if (tempA >= 9999) {
                        tempA = 0;
                    }
            }
        break;
    }
}

// ��ʱ��0�жϴ�����
void timer1_isr() interrupt 1 {
    static unsigned int j = 0;

    // ʱ���ֿ������
    if (hhFastTime < 3600)
        hhFastTime++;

    blinkTime++;
    if (blinkTime >= 1800)
        blinkTime = 0;
    
    if(playType != 0) {
        playTimer++;
    }
    
    j++;
    if (menu == MENU_ZIJIAN && hhFastTime >= 900) {
        playType = 1;
        playTime = 100;
        playTimer = 0;
        zijianpos++;
        if(zijianpos >= 8) menu = MENU_TIME;
        hhFastTime = 0;
    }
    if (j >= 3600) {
        j = 0;
        
        if(gear) {
            if(gearTime <= 0) gear = 0;
            else gearTime--;
        }
        
        if(triedTimes >= 3) {
            if(triedTimer == 0) triedTimes = 0;
            else triedTimer--;
        }

        //�롢�֡�ʱ�޸�*�簴���ڹ����޸ģ������ʾ����*
        sec++;
        updateDateTime(0, 0);
    }
}

// �������ɨ��
void key_scan() {
    unsigned int i;
    unsigned char line; //������
    unsigned char row;  //������

    // ���󰴼�ɨ��
    // �����洢
    scan_buf[0] = scan_buf[1]; //��̬����̬�����洢
    scan_buf[2] = scan_buf[3]; //�˲���Ԫ�����洢
    scan_buf[3] = scan_buf[4];

#ifdef U3CON
    // �����¼�ֵ
    //DISP_SEG = 0xff;          //����ʾ
    U3CON = 0x81;               //�У�PC7-PC4��Ϊ�������(PC3-PC0)Ϊ����
    VKEY_RL = 0x00;             //�����0 
    row = VKEY_RL;

    //DISP_SEG = 0xff;          //����ʾ
    U3CON = 0x88;               //�У�PC7-PC4��Ϊ���룬��(PC3-PC0)Ϊ���
    VKEY_RL = 0x00;             //�����0
    line = VKEY_RL;
#else
    DISP_SEG = 0xff; //����ʾ
    VKEY_RL = 0x0f;  //�����0
    delay_ms(1);
    row = VKEY_RL & 0x0f;
    DISP_SEG = 0xff; //����ʾ
    VKEY_RL = 0xf0;  //�����0
    delay_ms(1);
    line = VKEY_RL & 0xf0;
#endif
    row = row & 0x0f;         //���θ�4λ
    line = line & 0xf0;       // ���ε�4λ
    scan_buf[5] = line | row; //�ϲ�����ֵ

    // ���̱���-->��ֵ����scan_buf[4]
    for (i = 0; i < 16; i++) {
        if (scan_buf[5] == Scan_code[i]) {
            scan_buf[4] = i; //�����ɨ���룬ת���ɼ�ֵ0-15
            break;
        } else
            scan_buf[4] = 16; //�������Ϊ16
    }

    // �˲�
    if ((scan_buf[2] == scan_buf[3]) && (scan_buf[2] == scan_buf[4]))
        scan_buf[1] = scan_buf[4];
}

// ��������ļ��Ӽ����ܣ�����ѡʵ���ͻ��ֻ������һ��
int key_pro_4() {
    int i;
    // ���󰴼��¼�
    // ����
    if ((scan_buf[0] == 0x10) && (scan_buf[1] != 0x10)) {
        //������˵��������룬�޸�����
        if (menu == MENU_PWD || menu == MENU_PWD_CHANGE1 || menu == MENU_PWD_CHANGE2 || menu == MENU_PWD_CHANGE3) {
            if(triedTimes >= 3) {
                // �����޷���������
                playTime = 700;
                playTimer = 0;
                playType = 3;
                return 1;
            }
            
            if (inputLen < 6) {
                if (scan_buf[1] >= 1 && scan_buf[1] <= 3)
                    input[inputLen++] = scan_buf[1];
                else if (scan_buf[1] >= 5 && scan_buf[1] <= 7)
                    input[inputLen++] = scan_buf[1] - 1;
                else if (scan_buf[1] >= 9 && scan_buf[1] <= 11)
                    input[inputLen++] = scan_buf[1] - 2;
                else if (scan_buf[1] == 14)
                    input[inputLen++] = 0;
            }
            
            if (inputLen == 6) {
                if(menu == MENU_PWD_CHANGE1) {
                    for(i = 0; i < 6; i++)
                        if(pwd[i] != input[i]) break;
                    if(i==6) {
                        menu = MENU_PWD_CHANGE2;
                        inputLen = 0;
                        // ��������ɹ�����
                        playTime = 300;
                        playTimer = 0;
                        playType = 4;
                    } else {
                        // ���������������
                        playTime = 700;
                        playTimer = 0;
                        playType = 3;
                        triedTimes++;
                        if(triedTimes >= 3) {
                            // 30���Ӻ�������
                            triedTimer = 30;
                        }
                    }
                } else if(menu == MENU_PWD_CHANGE2) {
                    for(i = 0; i < 6; i++)
                        pwdBack[i] = input[i];
                    menu = MENU_PWD_CHANGE3;
                    inputLen = 0;
                    
                    // ���ŵ�һ������ɹ�����
                    playTime = 300;
                    playTimer = 0;
                    playType = 4;
                    
                } else if(menu == MENU_PWD_CHANGE3) {
                    for(i = 0; i < 6; i++)
                        if(pwdBack[i] != input[i]) break;
                    if(i==6) {
                        for(i = 0; i < 6; i++)
                            pwd[i] = input[i];
                        menu = MENU_TIME;
                        inputLen = 0;
                        // ��������ɹ�����
                        playTime = 400;
                        playTimer = 0;
                        playType = 4;
                    } else {
                        inputLen = 0;
                        // ���������������
                        playTime = 700;
                        playTimer = 0;
                        playType = 3;
                    }
                } else {
                    for(i = 0; i < 6; i++)
                        if(pwd[i] != input[i]) break;
                    if(i == 6) {
                        gear = 1;
                        gearTime = 3;  // ����3��
                        inputLen = 0;
                        
                        menu = MENU_TIME;
                        // ��������ɹ�����
                        playTime = 300;
                        playTimer = 0;
                        playType = 4;
                        triedTimes = 0;
                    } else {
                        // ���������������
                        playTime = 700;
                        playTimer = 0;
                        playType = 3;
                        triedTimes++;
                        if(triedTimes >= 3) {
                            // 30���Ӻ�������
                            triedTimer = 30;
                        }
                    }
                }
                inputLen = 0;
            }
        }
    }
    return 0;
}

int key_pro_zixuan() {
    unsigned char key;
    
    // ����
    if ((scan_buf[0] == 16) && (scan_buf[1] != 16)) {
        playType = 1;
        playTime = 200;
        playTimer = 0;
        
        switch (scan_buf[1]) {
        case 13:
            if(settime) settime = 0;
            else {
                menu++;
                if (menu > MENU_PWD)
                    menu = MENU_TIME;
                if (menu == MENU_PWD)
                    inputLen = 0;
            }
            return 1;
        case 15:
            if(settime == 1) {   // ����ʱ��
                hour = tempA;
                min = tempB;
                sec = tempC;
                settime = 0;
            } else if(settime == 2) {   // ��������
                year = tempA;
                month = tempB;
                day = tempC;
                settime = 0;
            } else if(hhFastTime < 1800) { // 0.2s
                // ˫���¼�
                menu = MENU_PWD_CHANGE1;
                inputLen = 0;
                return 1;
            } else {
                if(menu == MENU_TIME) {
                    settime = 1; // ����ʱ��
                    tempA = hour;
                    tempB = min;
                    tempC = sec;
                } else if(menu == MENU_DATE) {
                    settime = 2; // ��������
                    tempA = year;
                    tempB = month;
                    tempC = day;
                }
            }
        }
        if(settime) {
            if (1 <= scan_buf[1] && scan_buf[1] <= 3) {
                key = scan_buf[1];
            } else if (5 <= scan_buf[1] && scan_buf[1] <= 7) {
                key = scan_buf[1] - 1;
            } else if (9 <= scan_buf[1] && scan_buf[1] <= 11) {
                key = scan_buf[1] - 2;
            } else if (13 <= scan_buf[1] && scan_buf[1] <= 15) {
                key = scan_buf[1] - 3;
            }
            if (key < 10 || key == 11) {
                if (key == 11)
                    key = 0;
                switch (blinkIndex) {
                case 0:
                    if(settime == 1) {
                        // ����ʱʮλ
                        tempA = key * 10 + tempA % 10;
                    } else {
                        // ������ǧλ
                        tempA = tempA - (tempA / 1000 * 1000) + key * 1000;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 1:
                    if(settime == 1) {
                        // ����ʱ��λ
                        tempA = (tempA / 10) * 10 + key;
                        updateDateTime(settime, blinkIndex);
                        blinkIndex++;
                    } else {
                        // �������λ
                        tempA = (tempA / 1000 * 10 + key) * 100 + (tempA - tempA / 100 * 100);
                        updateDateTime(settime, blinkIndex);
                    }
                    blinkIndex++;
                    break;
                case 2:
                    if(settime == 1) {
                        ;
                    } else {
                        // ������ʮλ
                        tempA = (tempA / 100 * 10 + key) * 10 + tempA % 10;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 3:
                    if(settime == 1) {
                        // ���÷�ʮλ
                        tempB = key * 10 + tempB % 10;
                    } else {
                        // �������λ
                        tempA = tempA / 10 * 10 + key;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 4:
                    if(settime == 1) {
                        // ���÷ָ�λ
                        tempB = (tempB / 10) * 10 + key;
                        updateDateTime(settime, blinkIndex);
                        blinkIndex++;
                    } else {
                        // ������ʮλ
                        tempB = key * 10 + tempB % 10;
                        updateDateTime(settime, blinkIndex);
                    }
                    blinkIndex++;
                    break;
                case 5:
                    if(settime == 1) {
                        ;
                    } else {
                        // �����¸�λ
                        tempB = tempB / 10 * 10 + key;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 6:
                    if(settime == 1) {
                        // ������ʮλ
                        tempC = key * 10 + tempC % 10;
                    } else {
                        // ������ʮλ
                        tempC = key * 10 + tempC % 10;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 7:
                    if(settime == 1) {
                        // �������λ
                        tempC = (tempC / 10) * 10 + key;
                    } else {
                        // �����ո�λ
                        tempC = tempC / 10 * 10 + key;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex = 0;
                    break;
                }
            }
            
            if (scan_buf[1] == 0) {
                // �����ƶ�������ʱ��ʮλ
                blinkIndex--;
                if (settime == 1) {
                    if (blinkIndex == 2 || blinkIndex == 5)
                        blinkIndex--;
                }
            }
            if (scan_buf[1] == 4) {
                // �����ƶ������ø�λ����
                blinkIndex++;
                if (settime == 1) {
                    if (blinkIndex == 2 || blinkIndex == 5)
                        blinkIndex++;
                }
            }
            if (blinkIndex == 8)
                // ��λ���
                blinkIndex = 0;
            if (blinkIndex == -1)
                // �������ĩβ
                blinkIndex = 7;
        }
    }
    
    // �ɿ�
    if((scan_buf[0] != 0x10) && (scan_buf[1] == 0x10)) {
        switch(scan_buf[0]) {
            case 0:
                // ��ʼ��ʱ������
                hour = 23;
                min = 55;
                sec = 20;
                gear = 0;

                // ��ʼ������
                pwd[0] = 5;
                pwd[1] = 3;
                pwd[2] = 5;
                pwd[3] = 8;
                pwd[4] = 6;
                pwd[5] = 6;

                // ��ʼ��״̬
                settime = 0;
                triedTimes = 0;               // ����������������Ĵ���
                triedTimer = 0;               // �������������
                zijianpos = 0;
                playSound = 0;

                // �����Լ�˵�
                menu = MENU_ZIJIAN;
                break;
                
            case 15:
                // PB2 ��˫��LED�ɿ�
                if(!sgClick) {
                    sgClick = 1;
                    hhFastTime = 0; // ��ʱ����
                } else
                    sgClick = 0;
        }
    }
    return 0;
}

// ����������Ϊ����
void key_pro() {
    // ����ʱ������
    if(key_pro_zixuan()) return;

    // ��������
    if(key_pro_4()) return;
}

// ������
void alarm() {
    // ����������
    if(playType != 0) {
        if(playTime == 0) temp = 1;
        else temp = playTimer / playTime;
        if(temp % 2 == 0) playSound = 1;
        else playSound = 0;
        playTimer++;
        if(temp >= (playType << 1)) {
            playType = 0;
            playTimer = 0;
            playSound = 0;
        }
    }
    beep = !playSound;
}

void main() {
#ifdef U3CON
    U3CON = 0x89;   // 8255(U3)��ʼ����A��B�ڷ�ʽ0�����C������
#endif
    timer_init();
    delay_ms(60);

    // ��ʼ��ʱ������
    hour = 23;
    min = 55;
    sec = 20;
    gear = 0;
    
    // �����Լ�˵�
    menu = MENU_ZIJIAN;

    // ��ѭ��
    while (1) {
        // ˢ����ʾ
        display_buf();
        display_swbuf();
        
        // ���̼��
        key_scan();
        key_pro();

        // ����������
        alarm();
    }
}