/**
 ****************************************************************************************************
 * @file        netplay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-05-26
 * @brief       APP-网络测试 代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "netplay.h"
#include "gui.h"
#include "./T9INPUT/t9input.h"
#include "audioplay.h"
#include "./BSP/TPAD/tpad.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* CH395Q处理任务 */
#define CH395Q_HANDLER_TASK_PRIO    5       /* 设置任务优先级 */
#define CH395Q_HANDLER_STK_SIZE     64      /* 设置任务堆栈大小 */
OS_STK *CH395Q_HANDLER_TASK_STK;            /* 任务堆栈，采用内存管理的方式控制申请 */
void ch395q_handler_task(void *p_arg);      /* 任务函数 */

/* 静态网络信息定义(DHCP失败时使用) */
static uint8_t static_ip[4] = {192, 168, 1, 100};
static uint8_t static_gwip[4] = {192, 168, 1, 1};
static uint8_t static_mask[4] = {255, 255, 252, 0};

static struct
{
    uint8_t phy_status;                                             /* PHY连接状态 */
    uint8_t dhcp_status;                                            /* DHCP状态 */
    uint8_t tcp_conn_status[NET_TCP_SERVER_MAX_CONN + 1];           /* TCP(Server/Client)连接状态 */
    
    uint8_t socket_send_busy_status[NET_TCP_SERVER_MAX_CONN + 1];   /* Socket发送忙状态 */
    
    uint8_t *socket_recv_buf[NET_TCP_SERVER_MAX_CONN + 1];          /* Socket接收缓冲区 */
    uint8_t socket_recv_status[NET_TCP_SERVER_MAX_CONN + 1];        /* Socket接收有效状态 */
    uint16_t socket_recv_len[NET_TCP_SERVER_MAX_CONN + 1];          /* Socket接收有效数据大小 */
    
    uint8_t mac[6];                                                 /* MAC地址 */
    uint8_t ip[4];                                                  /* IP地址 */
    uint8_t gwip[4];                                                /* 网关IP地址 */
    uint8_t mask[4];                                                /* 子网掩码 */
    uint8_t dns1[4];                                                /* DNS服务器1地址 */
    uint8_t dns2[4];                                                /* DNS服务器2地址 */
    
    uint8_t des_ip[NET_TCP_SERVER_MAX_CONN + 1][4];                 /* 目标IP地址 */
    uint16_t des_port[NET_TCP_SERVER_MAX_CONN + 1];                 /* 目标端口号 */
} netdev = {0};

/* netplay提示信息 */
uint8_t *const netplay_remindmsg_tbl[5][GUI_LANGUAGE_NUM] =
{
    {"请插入网线!正在初始化网卡...", "請插入網線!正在初始化網卡...", "Pls insert cable!Ethernet Initing..",},
    {"初始化失败!请检查网线!", "初始化失敗!請檢查網線!", "Init failed!Check the cable!",},
    {"正在DHCP获取IP...", "正在DHCP獲取IP...", "DHCP IP configing...",},
    {"DHCP获取IP成功!", "DHCP獲取IP成功!", "DHCP IP config OK!",},
    {"DHCP获取IP失败,使用默认IP!", "DHCP獲取IP失敗,使用默認IP!", "DHCP IP config fail!Use default IP",},
};
/* netplay IP信息 */
uint8_t *const netplay_ipmsg[5][GUI_LANGUAGE_NUM] =
{
    {"本机MAC地址:", "本機MAC地址:", "Local MAC Addr:",},
    {" 远端IP地址:", " 遠端IP地址:", "Remote IP Addr:",},
    {" 本机IP地址:", " 本机IP地址:", " Local IP Addr:",},
    {"   子网掩码:", "   子網掩碼:", "   Subnet MASK:",},
    {"       网关:", "       網關:", "       Gateway:",},
};
/* 网速提示 */
uint8_t *const netplay_netspdmsg[GUI_LANGUAGE_NUM] = {"   网络速度:", "   網絡速度:", "Ethernet Speed:"};
/* netplay 测试提示信息 */
uint8_t *const netplay_testmsg_tbl[GUI_LANGUAGE_NUM] = {"可检查连接状态.", "可檢查連接狀態.", "to check the connection."};
/* netplay memo提示信息 */
uint8_t *const netplay_memoremind_tb[2][GUI_LANGUAGE_NUM] =
{
    {"接收区:", "接收區:", "Receive:",},
    {"发送区:", "發送區:", "Send:",},
};
/* netplay 测试按钮标题 */
uint8_t *const netplay_tbtncaption_tb[GUI_LANGUAGE_NUM] = {"开始测试", "開始測試", "Start Test",};
/* netplay 协议标题 */
uint8_t *const netplay_protcaption_tb[GUI_LANGUAGE_NUM] = {"协议", "協議", "PROT",};
/* netplay 协议名字 */
uint8_t *const netplay_protname_tb[3] = {"TCP Server", "TCP Client", "UDP",};
/* netplay 端口标题 */
uint8_t *const netplay_portcaption_tb[GUI_LANGUAGE_NUM] = {"端口:", "端口:", "Port:",};
/* netplay IP地址标题 */
uint8_t *const netplay_ipcaption_tb[2][GUI_LANGUAGE_NUM] =
{
    {"目标IP:", "目標IP:", "Target IP:",},
    {"本机IP:", "本機IP:", " Local IP:",},
};
/* netplay 按钮标题 */
uint8_t *const netplay_btncaption_tbl[5][GUI_LANGUAGE_NUM] =
{
    {"协议选择", "協議選擇", "PROT SEL",},
    {"连接", "連接", "Conn",},
    {"断开", "斷開", "Dis Conn",},
    {"清除接收", "清除接收", "Clear",},
    {"发送", "發送", "Send",},
};
/* 网络模式选择 */
uint8_t *const netplay_mode_tbl[3] = {"TCP Server", "TCP Client", "UDP"};
/* 网络连接提示信息 */
uint8_t *const netplay_connmsg_tbl[4][GUI_LANGUAGE_NUM] =
{
    {"正在连接...", "正在連接...", "Connecting...",},
    {"连接失败!", "連接失敗!", "Connect fail!",},
    {"连接成功!", "連接成功!", "Connect OK!",},
    {"连接断开!", "連接斷開!", "Disconnect!",},
};

/**
 * @brief       PHY连接回调
 * @param       phy_status: PHY状态
 * @retval      无
 */
static void phy_conn_cb(uint8_t phy_status)
{
    netdev.phy_status = phy_status;
}

/**
 * @brief       PHY断开回调
 * @param       无
 * @retval      无
 */
static void phy_disconn_cb(void)
{
    netdev.phy_status = CH395Q_CMD_PHY_DISCONN;
}

/**
 * @brief       DHCP成功回调
 * @param       ip  : IP地址
 *              gwip: 网关IP地址
 *              mask: 子网掩码
 *              dns1: DNS服务器1地址
 *              dns2: DNS服务器2地址
 * @retval      无
 */
static void dhcp_success_cb(uint8_t *ip, uint8_t *gwip, uint8_t *mask, uint8_t *dns1, uint8_t *dns2)
{
    uint8_t i;
    
    netdev.dhcp_status = 1;
    for (i=0; i<sizeof(netdev.ip); i++)
    {
        netdev.ip[i] = ip[i];
    }
    for (i=0; i<sizeof(netdev.gwip); i++)
    {
        netdev.gwip[i] = gwip[i];
    }
    for (i=0; i<sizeof(netdev.mask); i++)
    {
        netdev.mask[i] = mask[i];
    }
    for (i=0; i<sizeof(netdev.dns1); i++)
    {
        netdev.dns1[i] = dns1[i];
    }
    for (i=0; i<sizeof(netdev.dns2); i++)
    {
        netdev.dns2[i] = dns2[i];
    }
}

/**
 * @brief       Socket发送缓冲区空闲回调
 * @param       socket: Socket句柄
 * @retval      无
 */
static void socket_send_buf_free_cb(ch395q_socket_t *socket)
{
    netdev.socket_send_busy_status[socket->socket_index] = 0;   /* 标记发送缓冲区空闲，可以继续发送数据 */
}

/**
 * @brief       Socket接收缓冲区非空回调
 * @param       socket: Socket句柄
 * @retval      无
 */
static void socket_recv_cb(ch395q_socket_t *socket)
{
    uint16_t recv_len;
    
    recv_len = ch395q_cmd_get_recv_len_sn(socket->socket_index);                        /* 获取待接收数据长度 */
    if ((recv_len != 0) && (netdev.socket_recv_status[socket->socket_index] == 0))      /* 待接收数据长度不为0，已接收数据已被处理 */
    {
        if (recv_len > (socket->recv.size - 1))                                         /* 待接收数据长度超出接收缓冲区大小(为结束符预留1字节空间) */
        {
            recv_len = socket->recv.size;
        }
        ch395q_cmd_read_recv_buf_sn(socket->socket_index, recv_len, socket->recv.buf);  /* 接收数据 */
        netdev.socket_recv_status[socket->socket_index] = 1;                            /* 标记接收数据有效（待处理） */
        netdev.socket_recv_len[socket->socket_index] = recv_len;                        /* 保存已接收数据长度 */
    }
}

/**
 * @brief       Socket TCP连接回调
 * @param       socket: Socket句柄
 * @retval      无
 */
static void socket_tcp_conn_cb(ch395q_socket_t *socket)
{
    netdev.tcp_conn_status[socket->socket_index] = 1;           /* 标记TCP已连接 */
    netdev.des_ip[socket->socket_index][0] = socket->des_ip[0]; /* 保存目标IP地址和端口号 */
    netdev.des_ip[socket->socket_index][1] = socket->des_ip[1];
    netdev.des_ip[socket->socket_index][2] = socket->des_ip[2];
    netdev.des_ip[socket->socket_index][3] = socket->des_ip[3];
    netdev.des_port[socket->socket_index] = socket->des_port;
}

/**
 * @brief       Socket TCP断开回调
 * @param       socket: Socket句柄
 * @retval      无
 */
static void socket_tcp_disconn_cb(ch395q_socket_t *socket)
{
    netdev.tcp_conn_status[socket->socket_index] = 0;           /* 标记TCP未连接 */
}

/**
 * @brief       CH395Q处理任务
 * @param       p_arg: 未使用
 * @retval      无
 */
static void ch395q_handler_task(void *p_arg)
{
    while (1)
    {
        ch395q_handler();
        delay_ms(10);
    }
}

/**
 * @brief       创建CH395Q处理任务
 * @param       无
 * @retval      0   : 成功
 *              其他: 错误代码
 */
static uint8_t ch395q_handler_task_creat(void)
{
    OS_CPU_SR cpu_sr;
    uint8_t res;
    
    CH395Q_HANDLER_TASK_STK = (OS_STK *)gui_memin_malloc(CH395Q_HANDLER_STK_SIZE * sizeof(OS_STK));         /* 为CH395Q处理任务申请栈空间 */
    if (CH395Q_HANDLER_TASK_STK == NULL)                                                                    /* 内存申请失败 */
    {
        return 1;
    }
    
    OS_ENTER_CRITICAL();                                                                                    /* 进入临界区 */
    res = OSTaskCreate(ch395q_handler_task, (void *)0, (OS_STK *)&CH395Q_HANDLER_TASK_STK[CH395Q_HANDLER_STK_SIZE - 1], CH395Q_HANDLER_TASK_PRIO);
    OS_EXIT_CRITICAL();                                                                                     /* 退出临界区 */
    
    return res;
}

/**
 * @brief       删除CH395Q处理任务
 * @param       无
 * @retval      无
 */
static void ch395q_handler_task_delete(void)
{
    OSTaskDel(CH395Q_HANDLER_TASK_PRIO);        /* 删除CH395Q处理任务 */
    gui_memin_free(CH395Q_HANDLER_TASK_STK);    /* 释放内存 */
}

/**
 * @brief       加载网路测试主界面
 * @param       无
 * @retval      无
 */
static void net_load_ui(void)
{
    uint8_t *buf;
    uint8_t fsize = 0;
    uint16_t length;
    
    buf = gui_memin_malloc(100);
    app_filebrower((uint8_t *)APP_MFUNS_CAPTION_TBL[13][gui_phy.language], 0x05);                                                                   /* 显示标题 */
    
    if (lcddev.width == 240)                                                                                                                        /* 设置字体大小 */
    {
        fsize = 12;
    }
    else
    {
        fsize = 16;
    }
    
    length = strlen((char *)netplay_ipmsg[0][gui_phy.language]);                                                                                    /* 获取字符串长度，用于其他字符串的对齐 */
    gui_fill_rectangle(0, gui_phy.tbheight, lcddev.width, lcddev.height - gui_phy.tbheight, LGRAY);                                                 /* 填充主界面底色 */
    gui_show_string(netplay_ipmsg[0][gui_phy.language], 10, gui_phy.tbheight + 5, lcddev.width, fsize, fsize, BLUE);                                /* 本机MAC地址 */
    gui_show_string(netplay_ipmsg[2][gui_phy.language], 10, (4 + fsize) * 1 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, BLUE);              /* 本机IP地址 */
    gui_show_string(netplay_ipmsg[3][gui_phy.language], 10, (4 + fsize) * 2 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, BLUE);              /* 子网掩码 */
    gui_show_string(netplay_ipmsg[4][gui_phy.language], 10, (4 + fsize) * 3 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, BLUE);              /* 网关 */
    gui_show_string(netplay_netspdmsg[gui_phy.language], 10, (4 + fsize) * 4 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, BLUE);             /* 网络速度 */
    sprintf((char *)buf, "%02X-%02X-%02X-%02X-%02X-%02X", netdev.mac[0], netdev.mac[1], netdev.mac[2], netdev.mac[3], netdev.mac[4], netdev.mac[5]);
    gui_show_string(buf, 10 + length * fsize / 2, gui_phy.tbheight + 5, lcddev.width, fsize, fsize, RED);                                           /* 显示本机MAC地址 */
    sprintf((char *)buf, "%d.%d.%d.%d", netdev.ip[0], netdev.ip[1], netdev.ip[2], netdev.ip[3]);
    gui_show_string(buf, 10 + length * fsize / 2, (4 + fsize) * 1 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, RED);                         /* 显示本机IP地址 */
    sprintf((char *)buf, "%d.%d.%d.%d", netdev.mask[0], netdev.mask[1], netdev.mask[2], netdev.mask[3]);
    gui_show_string(buf, 10 + length * fsize / 2, (4 + fsize) * 2 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, RED);                         /* 显示子网掩码 */
    sprintf((char *)buf, "%d.%d.%d.%d", netdev.gwip[0], netdev.gwip[1], netdev.gwip[2], netdev.gwip[3]);
    gui_show_string(buf, 10 + length * fsize / 2, (4 + fsize) * 3 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, RED);                         /* 显示网关IP地址 */
    switch (netdev.phy_status)
    {
        case CH395Q_CMD_PHY_10M_FLL:
        {
            sprintf((char *)buf, "10Mbps Full");
            break;
        }
        case CH395Q_CMD_PHY_10M_HALF:
        {
            sprintf((char *)buf, "10Mbps Half");
            break;
        }
        case CH395Q_CMD_PHY_100M_FLL:
        {
            sprintf((char *)buf, "100Mbps Full");
            break;
        }
        case CH395Q_CMD_PHY_100M_HALF:
        {
            sprintf((char *)buf, "100Mbps Half");
            break;
        }
        case CH395Q_CMD_PHY_AUTO:
        {
            sprintf((char *)buf, "Auto");
            break;
        }
    }
    gui_show_string(buf, 10 + length * fsize / 2, (4 + fsize) * 4 + gui_phy.tbheight + 5, lcddev.width, fsize, fsize, RED);                         /* 显示网络速度 */
    sprintf((char *)buf, "Ping %d.%d.%d.%d %s", netdev.ip[0], netdev.ip[1], netdev.ip[2], netdev.ip[3], netplay_testmsg_tbl[gui_phy.language]);
    gui_show_string(buf, 10, (4 + fsize) * 6 + gui_phy.tbheight + 5, lcddev.width - 20, 2 * fsize, fsize, BLUE);                                    /* 显示Ping测试提示 */
    
    gui_memin_free(buf);
}

/**
 * @brief       显示提示信息
 * @param       y     : Y坐标（X坐标恒定从0开始）
 * @param       height: 区域高度
 * @param       fsize : 字体大小
 * @param       tx    : 发送字节数
 * @param       rx    : 接收字节数
 * @param       prot  : 协议类型
 *   @arg       0: TCP Server
 *   @arg       1: TCP Client
 *   @arg       2: UDP
 * @param       flag  : 更新标记
 *   @arg       bit0: 0: 不更新,1: 更新tx数据
 *   @arg       bit1: 0: 不更新,1: 更新rx数据
 *   @arg       bit2: 0: 不更新,1: 更新prot数据
 * @retval      无
 */
static void net_msg_show(uint16_t y, uint16_t height, uint8_t fsize, uint32_t tx, uint32_t rx, uint8_t prot, uint8_t flag)
{
    uint16_t xdis;
    uint8_t *pbuf;
    uint8_t i;
    uint32_t max;
    
    pbuf = gui_memin_malloc(100);
    if (pbuf == NULL)
    {
        return;
    }
    
    if (prot > 2)                                                                                                                               /* prot的范围为0~2 */
    {
        prot = 2;
    }
    
    xdis = (lcddev.width - (35 * fsize / 2)) / 3;
    
    if (flag & (1 << 0))                                                                                                                        /* 更新tx数据 */
    {
        gui_fill_rectangle(xdis / 2, y + (height - fsize) / 2, 10 * fsize / 2 + xdis, fsize, NET_MSG_BACK_COLOR);
        for (max=1, i=0; i<(10 + (2 * xdis / fsize)) - 3; i++)
        {
            max *= 10;
        }
        sprintf((char *)pbuf, "TX:%d%c", (tx <= (max - 1) ? tx : (max - 1)), (tx <= (max - 1) ? ' ' : '+'));
        gui_show_string(pbuf, xdis / 2, y + (height - fsize) / 2, lcddev.width, fsize, fsize, NET_MSG_FONT_COLOR);
    }
    
    if (flag & (1 << 1))                                                                                                                        /* 更新rx数据 */
    {
        gui_fill_rectangle(xdis / 2 + 10 * fsize / 2 + xdis, y + (height - fsize) / 2, 10 * fsize / 2 + xdis, fsize, NET_MSG_BACK_COLOR);
        for (max=1, i=0; i<(10 + (2 * xdis / fsize)) - 3; i++)
        {
            max *= 10;
        }
        sprintf((char *)pbuf, "RX:%d%c", (rx <= (max - 1) ? rx : (max - 1)), (rx <= (max - 1) ? ' ' : '+'));
        gui_show_string(pbuf, xdis / 2 + 10 * fsize / 2 + xdis, y + (height - fsize) / 2, lcddev.width, fsize, fsize, NET_MSG_FONT_COLOR);
    }
    
    if (flag & (1 << 2))                                                                                                                        /* 更新prot数据 */
    {
        gui_fill_rectangle(xdis / 2 + 20 * fsize / 2 + xdis * 2, y + (height - fsize) / 2, 15 * fsize / 2, fsize, NET_MSG_BACK_COLOR);
        sprintf((char *)pbuf, "%s:%s", netplay_protcaption_tb[gui_phy.language], netplay_protname_tb[prot]);
        gui_show_string(pbuf, xdis / 2 + 20 * fsize / 2 + xdis * 2, y + (height - fsize) / 2, lcddev.width, fsize, fsize, NET_MSG_FONT_COLOR);
    }
    
    gui_memin_free(pbuf);
}

/**
 * @brief       设置编辑框颜色
 * @param       ipx    : IP编辑框
 * @param       portx  : Port编辑框
 * @param       prot   : 协议
 *   @arg       0: TCP Server
 *   @arg       1: TCP Client
 *   @arg       2: UDP
 * @param       connsta: 连接状态
 *   @arg       0: 未连接
 *   @arg       1: 已连接
 * @retval      无
 */
static void net_edit_colorset(_edit_obj *ipx, _edit_obj *portx, uint8_t prot, uint8_t connsta)
{
    if (connsta == 1)                       /* 已连接，不可编辑 */
    {
        ipx->textcolor = WHITE;
        portx->textcolor = WHITE;
    }
    else                                    /* 未连接，可编辑 */
    {
        switch (prot)
        {
            case 0:                         /* TCP Server的协议可编辑，IP地址不可编辑 */
            {
                portx->textcolor = GREEN;
                ipx->textcolor = WHITE;
                break;
            }
            case 1:                         /* TCP Client的协议和IP地址可编辑 */
            case 2:                         /* UDP的协议和IP地址可编辑 */
            {
                portx->textcolor = GREEN;
                ipx->textcolor = GREEN;
                break;
            }
        }
    }
    
    edit_draw(ipx);                         /* 重绘编辑框 */
    edit_draw(portx);
}

/**
 * @brief       将字符串形式的端口号转换为数字形式的端口号
 * @param       str: 字符串形式的端口号
 * @retval      数字形式的端口号
 */
static uint16_t net_get_port(uint8_t *str)
{
    uint16_t port;
    
    port = atoi((char*)str);    /* 字符串转整数 */
    
    return port;
}

/**
 * @brief       将字符串形式的IP地址转换为数组形式的IP地址
 * @param       str: 字符串形式的IP地址
 * @param       ip : 数组形式的IP地址
 * @retval      转换结果
 *    @arg      0: 转换成功
 *    @arg      1: 字符串形式的IP地址有误，转换失败
 */
static uint8_t net_get_ip(uint8_t *str, uint8_t *ip)
{
    uint8_t *p1;
    uint8_t *p2;
    uint8_t *ipstr;
    
    ipstr = gui_memin_malloc(30);
    if(ipstr == NULL)
    {
        return 1;
    }
    strcpy((char *)ipstr, (char *)str);
    
    p1 = ipstr;                                     /* 获取第一个值 */
    p2 = (uint8_t *)strstr((const char *)p1, ".");
    if(p2 == NULL)
    {
        gui_memin_free(ipstr);
        return 1;
    }
    p2[0] = 0;
    ip[0] = atoi((char *)p1);
    
    p1 = p2 + 1;                                    /* 获取第二个值 */
    p2 = (uint8_t *)strstr((const char *)p1,".");
    if(p2 == NULL)
    {
        gui_memin_free(ipstr);
        return 1;
    }
    p2[0] = 0;
    ip[1] = atoi((char *)p1);
    
    p1 = p2 + 1;                                    /* 获取第三个值 */
    p2 = (uint8_t *)strstr((const char *)p1,".");
    if(p2 == NULL)
    {
        gui_memin_free(ipstr);
        return 1;
    }
    p2[0] = 0;
    ip[2] = atoi((char *)p1);
    
    p1 = p2 + 1;                                    /* 获取第四个值 */
    ip[3] = atoi((char *)p1);
    
    gui_memin_free(ipstr);
    
    return 0;
}

/**
 * @brief       网络测试主函数
 * @param       无
 * @retval      无
 */
static uint8_t net_test(void)
{
    uint8_t ip_height;              /* 界面布局相关变量 */
    uint8_t ip_fsize;
    uint8_t msg_height;
    uint16_t memo_width;
    uint16_t btn_width;
    uint16_t rmemo_height;
    uint16_t smemo_height;
    uint16_t rbtn_height;
    uint8_t m_offx;
    uint8_t sm_offy;
    uint8_t rm_offy;
    uint8_t fsize;
    uint8_t sbtnfsize;
    uint16_t t9height;
    uint16_t tempx;
    uint16_t tempy;
    uint8_t *ptemp;
    uint8_t *ipcaption;
    _edit_obj *eip;                 /* IP地址编辑框 */
    _edit_obj *eport;               /* 端口号编辑框 */
    _btn_obj *protbtn;              /* 协议选择按钮 */
    _btn_obj *connbtn;              /* 连接按钮 */
    _btn_obj *clrbtn;               /* 接收清除按钮 */
    _btn_obj *sendbtn;              /* 发送按钮 */
    _memo_obj *rmemo;               /* 接收区文本框 */
    _memo_obj *smemo;               /* 发送区文本框 */
    _t9_obj *t9;                    /* 输入法 */
    uint32_t rxcnt = 0;             /* 接收计数 */
    uint32_t txcnt = 0;             /* 发送计数 */
    uint8_t protocol = 0;           /* 协议标志
                                    * 0: TCP Server
                                    * 1: TCP Client
                                    * 2: UDP
                                    */
    uint8_t res;                    /* 按钮检测结果 */
    uint8_t rval = 0;               /* 控件等初始化结果 */
    uint16_t tport;                 /* 用于获取端口号 */
    uint8_t connstatus = 0;         /* 连接状态
                                    * 0: 未连接
                                    * 1: 已连接
                                    */
    uint8_t oldconnstatus = 0;      /* 记录上一次连接状态 */
    uint8_t oldaddr[4] = {0};       /* 记录上一次连接的目标IP地址 */
    uint16_t oldport = 0;           /* 记录上一次连接的目标端口号 */
    uint8_t editflag = 0;           /* 编辑标志
                                    * 0: 编辑发送区文本框
                                    * 1: 编辑IP地址编辑框
                                    * 2: 编辑端口号编辑框
                                    */
    uint16_t *bkcolor;              /* 保存被提示框遮挡的显示数据 */
    uint8_t retry;                  /* 错误重试计数 */
    ch395q_socket_t socket = {0};   /* CH395Q Socket结构体 */
    uint8_t conn_err;               /* Socket连接状态
                                     * 0: 连接成功
                                     * 1: 连接失败
                                     */
    uint8_t socket_index;           /* 用于遍历所有Socket */
    uint8_t socket_send;            /* 标记已向Socket发送数据 */
    
    lcd_clear(NET_MEMO_BACK_COLOR); /* 清屏 */
    if (lcddev.width == 240)        /* 根据LCD尺寸设置参数 */
    {
        ip_height = 20;
        ip_fsize = 12;
        msg_height = 16;
        memo_width = 172;
        btn_width = 56;
        rmemo_height = 72;
        smemo_height = 36;
        rbtn_height = 20;
        m_offx = 4;
        sm_offy = 4;
        rm_offy = 5;
        fsize = 12;
        sbtnfsize = 16;
        t9height = 134;
    }
    else if (lcddev.width == 272)
    {
        ip_height = 20;
        ip_fsize = 12;
        msg_height = 20;
        memo_width = 180;
        btn_width = 68;
        rmemo_height = 160;
        smemo_height = 48;
        rbtn_height = 34;
        m_offx = 4;
        sm_offy = 8;
        rm_offy = 8;
        fsize = 12;
        sbtnfsize = 16;
        t9height = 176;
    }
    else if (lcddev.width == 320)
    {
        ip_height = 24;
        ip_fsize = 16;
        msg_height = 20;
        memo_width = 208;
        btn_width = 80;
        rmemo_height = 144;
        smemo_height = 48;
        rbtn_height = 40;
        m_offx = 12;
        sm_offy = 8;
        rm_offy = 10;
        fsize = 16;
        sbtnfsize = 24;
        t9height = 176;
    }
    else if (lcddev.width == 480)
    {
        ip_height = 36;
        ip_fsize = 24;
        msg_height = 28;
        memo_width = 304;
        btn_width = 100;
        rmemo_height = 336;
        smemo_height = 64;
        rbtn_height = 50;
        m_offx = 16;
        sm_offy = 10;
        rm_offy = 9;
        fsize = 16;
        sbtnfsize = 24;
        t9height = 266;
    }
    else if (lcddev.width == 600)
    {
        ip_height = 36;
        ip_fsize = 24;
        msg_height = 28;
        memo_width = 364;
        btn_width = 120;
        rmemo_height = 436;
        smemo_height = 80;
        rbtn_height = 50;
        m_offx = 16;
        sm_offy = 12;
        rm_offy = 10;
        fsize = 16;
        sbtnfsize = 24;
        t9height = 368;
    }
    else if (lcddev.width == 800)
    {
        ip_height = 60;
        ip_fsize = 32;
        msg_height = 30;
        memo_width = 500;
        btn_width = 200;
        rmemo_height = 480;
        smemo_height = 150;
        rbtn_height = 50;
        m_offx = 16;
        sm_offy = 10;
        rm_offy = 10;
        fsize = 16;
        sbtnfsize = 24;
        t9height = 488;
    }
    
    gui_fill_rectangle(0, 0, lcddev.width, ip_height, NET_IP_BACK_COLOR);                                                                                       /* 填充IP地址、端口号区域背景 */
    gui_fill_rectangle(0, ip_height, lcddev.width, msg_height, NET_MSG_BACK_COLOR);                                                                             /* 填充统计信息、协议区域背景 */
    gui_draw_hline(0, ip_height + msg_height - 1, lcddev.width, NET_COM_RIM_COLOR);                                                                             /* 接收区上方的分割线 */
    tempy = ip_height + msg_height + rmemo_height + fsize + 2 * rm_offy;
    gui_draw_hline(0, tempy, lcddev.width, NET_COM_RIM_COLOR);                                                                                                  /* 发送区上方的分割线 */
    tempx = (lcddev.width - 35 * ip_fsize / 2) / 3;
    ipcaption = netplay_ipcaption_tb[1][gui_phy.language];
    gui_show_string(ipcaption, tempx, (ip_height - ip_fsize) / 2, lcddev.width, ip_fsize, ip_fsize, WHITE);                                                     /* 显示IP地址标题文本 */
    tempx = lcddev.width - tempx - 10 * ip_fsize / 2;
    gui_show_string(netplay_portcaption_tb[gui_phy.language], tempx, (ip_height - ip_fsize) / 2, lcddev.width, ip_fsize, ip_fsize, WHITE);                      /* 显示端口号标题文本 */
    tempy = ip_height + msg_height + rm_offy + fsize;
    gui_show_string(netplay_memoremind_tb[0][gui_phy.language], m_offx, tempy - fsize - rm_offy / 3, lcddev.width, fsize, fsize, NET_MSG_FONT_COLOR);           /* 显示接收区标题文本 */
    rmemo = memo_creat(m_offx, tempy, memo_width, rmemo_height, 0, 0, fsize, NET_RMEMO_MAXLEN);                                                                 /* 创建接收区文本框 */
    tempx = lcddev.width - tempx - 10 * ip_fsize / 2;
    eip = edit_creat(strlen((char *)ipcaption) * ip_fsize / 2 + tempx, (ip_height - ip_fsize - 6) / 2, 15 * ip_fsize / 2 + 6, ip_fsize + 6, 0, 4, ip_fsize);    /* 创建IP地址编辑框 */
    tempx = (lcddev.width - 35 * ip_fsize / 2) / 3;
    tempx = lcddev.width - tempx - 10 * ip_fsize / 2;
    eport = edit_creat(tempx + 5 * ip_fsize / 2, (ip_height - ip_fsize - 6) / 2, 5 * ip_fsize / 2 + 6, ip_fsize + 6, 0, 4, ip_fsize);                           /* 创建端口号编辑框 */
    tempy = ip_height + msg_height + rm_offy * 2 + rmemo_height + fsize * 2 + sm_offy;
    gui_show_string(netplay_memoremind_tb[1][gui_phy.language], m_offx, tempy - fsize - sm_offy / 3, lcddev.width, fsize, fsize, NET_MSG_FONT_COLOR);           /* 显示发送区标题文本 */
    smemo = memo_creat(m_offx, tempy, memo_width, smemo_height, 0, 1, fsize, NET_SMEMO_MAXLEN);                                                                 /* 创建发送区文本框 */
    tempy = ip_height + msg_height + rm_offy * 2 + rmemo_height + fsize * 2 + sm_offy * 2 + smemo_height;
    t9 = t9_creat((lcddev.width % 5) / 2, tempy, lcddev.width - (lcddev.width % 5), t9height, 0);                                                               /* 创建输入法 */
    tempy = ip_height + msg_height + rm_offy + fsize;
    tempx = (lcddev.width - (m_offx + memo_width + btn_width)) / 2 + m_offx + memo_width;
    protbtn = btn_creat(tempx, tempy, btn_width, rbtn_height, 0, 0);                                                                                            /* 创建协议选择按钮 */
    memo_width = (rmemo_height - 3 * rbtn_height) / 2;
    if (memo_width > rbtn_height / 2)
    {
        memo_width = rbtn_height / 2;
    }
    connbtn = btn_creat(tempx, tempy + memo_width + rbtn_height, btn_width, rbtn_height, 0, 0);                                                                 /* 创建连接按钮 */
    clrbtn = btn_creat(tempx, tempy + memo_width * 2 + rbtn_height * 2, btn_width, rbtn_height, 0, 0);                                                          /* 创建清除接收按钮 */
    tempy = ip_height + msg_height + rm_offy * 2 + rmemo_height + fsize * 2 + sm_offy;
    sendbtn = btn_creat(tempx, tempy, btn_width, smemo_height, 0, 2);                                                                                           /* 创建发送按钮 */
    ptemp = gui_memin_malloc(100);
    
    if (!rmemo || !eip || !eport || !smemo || !t9 || !protbtn || !connbtn || !clrbtn || !sendbtn || !ptemp)                                                     /* 若有控件创建失败，则返回 */
    {
        rval = 1;
    }
    
    if (rval == 0)                                                                                      /* 控件创建成功 */
    {
        protbtn->caption = netplay_btncaption_tbl[0][gui_phy.language];                                 /* 设置协议选择按钮的样式 */
        protbtn->font = fsize;
        connbtn->caption = netplay_btncaption_tbl[1][gui_phy.language];                                 /* 设置连接按钮的样式 */
        connbtn->font = fsize;
        clrbtn->caption = netplay_btncaption_tbl[3][gui_phy.language];                                  /* 设置清除接收按钮的样式 */
        clrbtn->font = fsize;
        sendbtn->bkctbl[0] = 0x6BF6;                                                                    /* 设置发送按钮的样式 */
        sendbtn->bkctbl[1] = 0x545E;
        sendbtn->bkctbl[2] = 0x5C7E;
        sendbtn->bkctbl[3] = 0x2ADC;
        sendbtn->bcfucolor = WHITE;
        sendbtn->bcfdcolor = BLACK;
        sendbtn->caption = netplay_btncaption_tbl[4][gui_phy.language];
        sendbtn->font = sbtnfsize;
        eip->textbkcolor = NET_IP_BACK_COLOR;                                                           /* 设置IP地址编辑框的样式 */
        eip->textcolor = WHITE;
        eport->textbkcolor = NET_IP_BACK_COLOR;                                                         /* 设置端口号编辑框的样式 */
        eport->textcolor = GREEN;       
        rmemo->textbkcolor = WHITE;                                                                     /* 设置接收区文本框的样式 */
        rmemo->textcolor = BLACK;
        smemo->textbkcolor = WHITE;                                                                     /* 设置发送区文本框的样式 */
        smemo->textcolor = BLACK;
        sprintf((char *)ptemp, "%d.%d.%d.%d", netdev.ip[0], netdev.ip[1], netdev.ip[2], netdev.ip[3]);
        strcpy((char *)eip->text, (const char *)ptemp);                                                 /* 设置IP地址编辑框显示本地IP地址 */
        tport = 8088;
        sprintf((char *)ptemp, "%d", tport);
        strcpy((char *)eport->text, (const char *)ptemp);                                               /* 设置端口号编辑框显示默认端口号 */
        edit_draw(eip);                                                                                 /* 重绘IP地址编辑框 */
        edit_draw(eport);                                                                               /* 重绘端口号编辑框 */
        memo_draw_memo(smemo, 0);                                                                       /* 重绘发送区文本框 */
        memo_draw_memo(rmemo, 0);                                                                       /* 重绘接收区文本框 */
        btn_draw(protbtn);                                                                              /* 重绘协议选择按钮 */
        btn_draw(connbtn);                                                                              /* 重绘连接按钮 */
        btn_draw(clrbtn);                                                                               /* 重绘接收清除按钮 */
        btn_draw(sendbtn);                                                                              /* 重绘发送按钮 */
        t9_draw(t9);                                                                                    /* 重绘输入法 */
        net_msg_show(ip_height, msg_height, fsize, txcnt, rxcnt, protocol, 0x07);                       /* 初始化显示提示信息 */
    }
    
    while (rval == 0)
    {
        tp_dev.scan(0);                             /* 触摸屏扫描 */
        in_obj.get_key(&tp_dev, IN_TYPE_TOUCH);     /* 获取触摸信息 */
        delay_ms(1000 / OS_TICKS_PER_SEC);
        
        if (system_task_return)                     /* 强制退出有效 */
        {
            delay_ms(15);
            if (tpad_scan(1))                       /* 再次确认TPAD，排除干扰 */
            {
                break;                              /* 退出 */
            }
            else
            {
                system_task_return = 0;             /* 清除强制退出标志 */
            }
        }
        
        if (connstatus == 0)                                                                                                                                    /* 在未连接的情况下，可以编辑发送区文本框、IP地址编辑框、端口号编辑框 */
        {
            if ((smemo->top < in_obj.y) && (in_obj.y < (smemo->top + smemo->height)) && (smemo->left < in_obj.x) && (in_obj.x < (smemo->left + smemo->width)))
            {
                editflag = 0;                                                                                                                                   /* 标记编辑发送区文本框 */
                edit_show_cursor(eip, 0);                                                                                                                       /* 不显示另外两处(IP地址编辑框、端口号编辑框)的光标 */
                edit_show_cursor(eport, 0);
                eip->type = 0x04;                                                                                                                               /* 设置IP地址编辑框的光标不闪烁 */
                eport->type = 0x04;                                                                                                                             /* 设置端口号编辑框的光标不闪烁 */
                smemo->type = 0x01;                                                                                                                             /* 允许编辑发送区文本框 */
            }
            
            if ((eip->top < in_obj.y) && (in_obj.y < (eip->top + eip->height)) && (eip->left < in_obj.x) && (in_obj.x < (eip->left + eip->width)))
            {
                if (protocol == 0)                                                                                                                              /* TCP Server协议下不可编辑IP地址 */
                {
                    continue;
                }

                editflag = 1;                                                                                                                                   /* 标记编辑IP地址编辑框 */
                memo_show_cursor(smemo, 0);                                                                                                                     /* 不显示另外两处(发送区文本框、端口号编辑框)的光标 */
                edit_show_cursor(eport, 0);
                eip->type = 0x06;                                                                                                                               /* 设置IP地址编辑框的光标闪烁 */
                eport->type = 0x04;                                                                                                                             /* 设置端口号编辑框的光标不闪烁 */
                smemo->type = 0x00;                                                                                                                             /* 不允许编辑发送区文本框 */
            }
            
            if ((eport->top < in_obj.y) && (in_obj.y < (eport->top + eport->height)) && (eport->left < in_obj.x) && (in_obj.x < (eport->left + eport->width)))
            {
                editflag = 2;                                                                                                                                   /* 标记编辑端口号编辑框 */
                memo_show_cursor(smemo, 0);                                                                                                                     /* 不显示另外两处(发送区文本框、IP地址编辑框)的光标 */
                edit_show_cursor(eip, 0);
                eport->type = 0x06;                                                                                                                             /* 设置端口号编辑框的光标闪烁 */
                eip->type = 0x04;                                                                                                                               /* 设置IP地址编辑框的光标不闪烁 */
                smemo->type = 0x00;                                                                                                                             /* 不允许编辑发送区文本框 */
            }
        }
        
        edit_check(eip, &in_obj);       /* 检查IP地址编辑框 */
        edit_check(eport, &in_obj);     /* 检查端口号编辑框 */
        t9_check(t9, &in_obj);          /* 检查输入法 */
        memo_check(smemo, &in_obj);     /* 检查发送区文本框 */
        memo_check(rmemo, &in_obj);     /* 检查接收区文本框 */
        
        if (t9->outstr[0] != NULL)                                                                                      /* 输入法有字符输出 */
        {
            if (editflag == 1)                                                                                          /* 编辑IP地址编辑框 */
            {
                if ((t9->outstr[0] <= '9' && t9->outstr[0] >= '0') || t9->outstr[0] == '.' || t9->outstr[0] == 0x08)    /* IP地址编辑框只接受数字0~9、“.”和退格 */
                {
                    edit_add_text(eip, t9->outstr);                                                                     /* 添加文本至IP地址编辑框 */
                }
            }
            else if (editflag == 2)                                                                                     /* 编辑端口号编辑框 */
            {
                if ((t9->outstr[0] <= '9' && t9->outstr[0] >= '0') || t9->outstr[0] == 0x08)                            /* 端口号编辑框只接受数字0~9和退格 */
                {
                    edit_add_text(eport, t9->outstr);                                                                   /* 添加文本至端口号编辑框 */
                }
            }
            else                                                                                                        /* 编辑发送区文本框 */
            {
                memo_add_text(smemo, t9->outstr);                                                                       /* 添加文本至发送区文本框 */
            }
            
            t9->outstr[0] = NULL;
        }
        
        res = btn_check(protbtn, &in_obj);                                                                                                      /* 检查协议选择按钮 */
        if (res && ((protbtn->sta & (1 << 7)) == 0) && (protbtn->sta & (1 << 6)))                                                               /* 协议选择按钮被按下并松开 */
        {
            tempx = protocol;                                                                                                                   /* 获取当前已选择的协议 */
            res = app_items_sel((lcddev.width - 180) / 2,                                                                                             /* 进入协议选择 */
                                (lcddev.height - 192) / 2,
                                180,
                                72 + 40 * 3,
                                (uint8_t **)netplay_mode_tbl,
                                3,
                                (uint8_t *)&tempx,
                                0xD0,
                                (uint8_t *)netplay_btncaption_tbl[0][gui_phy.language]);

            if ((tempx != protocol) && (res == 0))  /* 确认按钮按下了, 且选择了其他协议 */
            {
                protocol = tempx;                                                                                                               /* 更改协议类型 */
                if (protocol != 0)                                                                                                              /* TCP Client和UDP可设置目标IP地址 */
                {
                    ipcaption = netplay_ipcaption_tb[0][gui_phy.language];
                }
                else                                                                                                                            /* TCP Server只能显示本地IP地址 */
                {
                    sprintf((char *)ptemp, "%d.%d.%d.%d", netdev.ip[0], netdev.ip[1], netdev.ip[2], netdev.ip[3]);
                    strcpy((char *)eip->text, (const char *)ptemp);
                    ipcaption = netplay_ipcaption_tb[1][gui_phy.language];
                }
                
                tempx = (lcddev.width - 35 * ip_fsize / 2) / 3;
                gui_fill_rectangle(tempx, (ip_height - ip_fsize) / 2, ip_fsize * strlen((char *)ipcaption) / 2, ip_fsize, NET_IP_BACK_COLOR);   /* 修改IP地址标题文本 */
                gui_show_string(ipcaption, tempx, (ip_height - ip_fsize) / 2, lcddev.width, ip_fsize, ip_fsize, WHITE);
                net_edit_colorset(eip, eport, protocol, connstatus);                                                                            /* 重绘IP地址编辑框 */
                net_msg_show(ip_height, msg_height, fsize, txcnt, rxcnt, protocol, 1 << 2);                                                     /* 更新协议信息 */
            }
        }
        
        res = btn_check(connbtn, &in_obj);                                                                  /* 检查连接按钮 */
        if (res && ((connbtn->sta & (1 << 7)) == 0) && (connbtn->sta & (1 << 6)))                           /* 连接按钮被按下并松开 */
        {
            connstatus = 1 - connstatus;                                                                    /* 切换连接标志 */
            
            if (connstatus == 1)                                                                            /* 需要建立连接 */
            {
                bkcolor = gui_memex_malloc(200 * 80 * 2);
                if (bkcolor == NULL)
                {
                    connstatus = 0;
                    printf("netplay ex outof memory\r\n");
                    continue;
                }
                app_read_bkcolor((lcddev.width - 200) / 2, (lcddev.height - 80) / 2, 200, 80, bkcolor);     /* 预先读取被提提示挡住图显示数据 */
                window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 80) / 2,                          /* 显示正在连接提示框 */
                               200, 80,
                               (uint8_t *)netplay_connmsg_tbl[0][gui_phy.language],
                               (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                               12, 0, 0, 0);
                
                conn_err = 0;
                netdev.socket_recv_buf[0] = gui_memex_malloc(NET_RECV_BUF_MAX_LEN);                         /* 为CH395Q Socket 0数据接收缓冲区申请内存 */
                if (netdev.socket_recv_buf[0] == NULL)                                                      /* 内存申请失败，则不能进行连接 */
                {
                    conn_err = 1;                                                                           /* 标记连接失败 */
                }
                else
                {
                    switch (protocol)                                                                       /* 根据不同协议配置Socket */
                    {
                        case 0:                                                                             /* TCP Server */
                        {
                            netdev.tcp_conn_status[0] = 0;                                                  /* 标记TCP为未连接 */
                            tport = net_get_port(eport->text);                                              /* 获取端口号 */
                            socket.socket_index = CH395Q_SOCKET_0;                                          /* Socket 0 */
                            socket.enable = CH395Q_ENABLE;                                                  /* 使能Socket */
                            socket.proto = CH395Q_SOCKET_TCP_SERVER;                                        /* TCP Server协议 */
                            socket.sour_port = tport;                                                       /* 本地端口号 */
                            socket.send.buf = smemo->text;                                                  /* 发送缓冲区 */
                            socket.send.size = NET_SMEMO_MAXLEN;                                            /* 发送缓冲区大小 */
                            socket.recv.buf =  netdev.socket_recv_buf[0];                                   /* 接收缓冲区 */
                            socket.recv.size = NET_RECV_BUF_MAX_LEN;                                        /* 接收缓冲区大小 */
                            socket.send_buf_free_cb = socket_send_buf_free_cb;                              /* 发送缓冲区空闲回调 */
                            socket.recv_cb = socket_recv_cb;                                                /* 接收缓冲区非空回调 */
                            socket.tcp_conn_cb = socket_tcp_conn_cb;                                        /* TCP连接回调 */
                            socket.tcp_disconn_cb = socket_tcp_disconn_cb;                                  /* TCP断开回调 */
                            ch395q_socket_config(&socket);                                                  /* 配置Socket */
                            conn_err = 0;                                                                   /* 标记连接成功 */
                            for (socket_index=1; socket_index<NET_TCP_SERVER_MAX_CONN + 1; socket_index++)  /* 配置TCP Server多连接 */
                            {
                                netdev.tcp_conn_status[socket_index] = 0;
                                netdev.socket_recv_buf[socket_index] = gui_memex_malloc(NET_RECV_BUF_MAX_LEN);
                                if (netdev.socket_recv_buf[socket_index] == NULL)
                                {
                                    conn_err = 1;
                                    break;
                                }
                                socket.socket_index = socket_index;
                                socket.recv.buf = netdev.socket_recv_buf[socket_index];
                                ch395q_socket_config(&socket);
                            }
                            break;
                        }
                        case 1:                                                                             /* TCP Client */
                        {
                            netdev.tcp_conn_status[0] = 0;                                                  /* 标记TCP为未连接 */
                            tport = net_get_port(eport->text);                                              /* 获取端口号 */
                            socket.socket_index = CH395Q_SOCKET_0;                                          /* Socket 0 */
                            socket.enable = CH395Q_ENABLE;                                                  /* 使能Socket */
                            socket.proto = CH395Q_SOCKET_TCP_CLIENT;                                        /* TCP Client协议 */
                            net_get_ip(eip->text, socket.des_ip);                                           /* 目标IP地址 */
                            socket.des_port = tport;                                                        /* 目标端口号 */
                            socket.sour_port = tport;                                                       /* 本地端口号 */
                            socket.send.buf = smemo->text;                                                  /* 发送缓冲区 */
                            socket.send.size = NET_SMEMO_MAXLEN;                                            /* 发送缓冲区大小 */
                            socket.recv.buf =  netdev.socket_recv_buf[0];                                   /* 接收缓冲区 */
                            socket.recv.size = NET_RECV_BUF_MAX_LEN;                                        /* 接收缓冲区大小 */
                            socket.send_buf_free_cb = socket_send_buf_free_cb;                              /* 发送缓冲区空闲回调 */
                            socket.recv_cb = socket_recv_cb;                                                /* 接收缓冲区非空回调 */
                            socket.tcp_conn_cb = socket_tcp_conn_cb;                                        /* TCP连接回调 */
                            socket.tcp_disconn_cb = socket_tcp_disconn_cb;                                  /* TCP断开回调 */
                            ch395q_socket_config(&socket);                                                  /* 配置Socket */
                            retry = 50;
                            while ((netdev.tcp_conn_status[0] == 0) && (retry != 0))                        /* 等待连接至TCP Server */
                            {
                                delay_ms(100);
                                retry--;
                            }
                            if(netdev.tcp_conn_status[0] != 0)                                              /* TCP连接成功 */
                            {
                                conn_err = 0;
                                netdev.des_ip[0][0] = socket.des_ip[0];                                     /* 保存目标IP地址和端口号 */
                                netdev.des_ip[0][1] = socket.des_ip[1];
                                netdev.des_ip[0][2] = socket.des_ip[2];
                                netdev.des_ip[0][3] = socket.des_ip[3];
                                netdev.des_port[0] = socket.des_port;
                            }
                            else                                                                            /* TCP连接失败 */
                            {
                                conn_err = 1;                                                               /* 标记连接失败 */
                            }
                            break;
                        }
                        case 2:                                                                             /* UDP */
                        {
                            tport = net_get_port(eport->text);                                              /* 获取端口号 */
                            socket.socket_index = CH395Q_SOCKET_0;                                          /* Socket 0 */
                            socket.enable = CH395Q_ENABLE;                                                  /* 使能Socket */
                            socket.proto = CH395Q_SOCKET_UDP;                                               /* UDP协议 */
                            net_get_ip(eip->text, socket.des_ip);                                           /* 目标IP地址 */
                            socket.des_port = tport;                                                        /* 目标端口号 */
                            socket.sour_port = tport;                                                       /* 本地端口号 */
                            socket.send.buf = smemo->text;                                                  /* 发送缓冲区 */
                            socket.send.size = NET_SMEMO_MAXLEN;                                            /* 发送缓冲区大小 */
                            socket.recv.buf =  netdev.socket_recv_buf[0];                                   /* 接收缓冲区 */
                            socket.recv.size = NET_RECV_BUF_MAX_LEN;                                        /* 接收缓冲区大小 */
                            socket.send_buf_free_cb = socket_send_buf_free_cb;                              /* 发送缓冲区空闲回调 */
                            socket.recv_cb = socket_recv_cb;                                                /* 接收缓冲区非空回调 */
                            ch395q_socket_config(&socket);                                                  /* 配置Socket */
                            conn_err = 0;
                            netdev.des_ip[0][0] = socket.des_ip[0];                                         /* 保存目标IP地址和端口号 */
                            netdev.des_ip[0][1] = socket.des_ip[1];
                            netdev.des_ip[0][2] = socket.des_ip[2];
                            netdev.des_ip[0][3] = socket.des_ip[3];
                            netdev.des_port[0] = socket.des_port;
                            break;
                        }
                    }
                }
                
                if (conn_err == 0)                                                                          /* 连接成功 */
                {
                    window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 80) / 2,                      /* 显示连接成功提示框 */
                                   200, 80,
                                   (uint8_t *)netplay_connmsg_tbl[2][gui_phy.language],
                                   (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                                   12, 0, 0, 0);
                }
                else                                                                                        /* 连接失败 */
                {
                    connstatus = 0;                                                                         /* 标记为未连接 */
                    for (socket_index=0; socket_index<NET_TCP_SERVER_MAX_CONN + 1; socket_index++)
                    {
                        socket.socket_index = socket_index;
                        socket.enable = CH395Q_DISABLE;
                        ch395q_socket_config(&socket);                                                      /* 关闭Socket */
                        gui_memex_free(netdev.socket_recv_buf[socket_index]);                               /* 释放Socket数据接收缓冲区 */
                    }
                    window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 80) / 2,                      /* 显示连接失败提示框 */
                                   200, 80,
                                   (uint8_t *)netplay_connmsg_tbl[1][gui_phy.language],
                                   (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                                   12, 0, 0, 0);
                }
                
                delay_ms(800);
                app_recover_bkcolor((lcddev.width - 200) / 2, (lcddev.height - 80) / 2, 200, 80, bkcolor);  /* 恢复被提示框挡住的显示数据 */
                gui_memex_free(bkcolor);
            }
        }
        
        res = btn_check(clrbtn, &in_obj);                                               /* 检查接收清除按钮 */
        if (res && ((clrbtn->sta & (1 << 7)) == 0) && (clrbtn->sta & (1 << 6)))         /* 接收清除按钮被按下并松开 */
        {
            rxcnt = 0;                                                                  /* 重置接收计数 */
            txcnt = 0;                                                                  /* 重置发送计数 */
            rmemo->text[0] = 0;                                                         /* 清空接收区文本框内容 */
            memo_draw_memo(rmemo, 1);                                                   /* 重绘接收区文本框 */
            net_msg_show(ip_height, msg_height, fsize, txcnt, rxcnt, protocol, 0x07);   /* 更新统计信息 */
        }
        
        res = btn_check(sendbtn, &in_obj);                                                                                  /* 检查发送按钮 */
        if (res && ((sendbtn->sta & (1 << 7)) == 0) && (sendbtn->sta & (1 << 6)))                                           /* 发送按钮被按下并松开 */
        {
            tempx = strlen((char *)smemo->text);                                                                            /* 获取发送区文本框文本长度 */
            if ((connstatus == 1) && (tempx != 0))                                                                          /* 发送条件为处于连接状态、待发送数据长度不为0 */
            {
                socket_send = 0;
                for (socket_index=0; socket_index<((protocol == 0) ? (NET_TCP_SERVER_MAX_CONN + 1) : 1); socket_index++)    /* 遍历Socket */
                {
                    if (netdev.socket_send_busy_status[socket_index] == 0)                                                  /* 当前Socket已发送完毕 */
                    {
                        netdev.socket_send_busy_status[socket_index] = 1;                                                   /* 标记当前Socket有数据需要发送 */
                        ch395q_cmd_write_send_buf_sn(socket_index, smemo->text, tempx);                                     /* 向当前Socket的发送缓冲区写数据 */
                        if (socket_send == 0)
                        {
                            socket_send = 1;
                            txcnt += strlen((char *)smemo->text);
                            net_msg_show(ip_height, msg_height, fsize, txcnt, rxcnt, protocol, 1 << 0);
                        }
                    }
                }
            }
        }
        
        if ((connstatus == 1) && (protocol == 1))          /* 若打开了TCP Client连接，则是否连接成功取决于TCP的连接状态 */
        {
            connstatus = netdev.tcp_conn_status[0];
        }
        
        if (netdev.phy_status == CH395Q_CMD_PHY_DISCONN)    /* 若PHY断开，则标记为未连接状态 */
        {
            connstatus = 0;
        }
        
        if (connstatus == 1)                                                                                                            /* 已连接状态 */
        {
            for (socket_index=0; socket_index<((protocol == 0) ? NET_TCP_SERVER_MAX_CONN + 1 : 1); socket_index++)                      /* 遍历Socket */
            {
                if (netdev.socket_recv_status[socket_index] == 1)                                                                       /* 接收到数据 */
                {
                    if (((netdev.des_ip[socket_index][0] != oldaddr[0]) ||
                         (netdev.des_ip[socket_index][1] != oldaddr[1]) ||
                         (netdev.des_ip[socket_index][2] != oldaddr[2]) ||
                         (netdev.des_ip[socket_index][3] != oldaddr[3])) ||                                                             /* 新的目标IP地址或端口号 */
                        (netdev.des_port[socket_index] != oldport))
                    {
                        oldaddr[0] = netdev.des_ip[socket_index][0];                                                                    /* 保存新的目标IP地址和端口号 */
                        oldaddr[1] = netdev.des_ip[socket_index][1];
                        oldaddr[2] = netdev.des_ip[socket_index][2];
                        oldaddr[3] = netdev.des_ip[socket_index][3];
                        oldport = netdev.des_port[socket_index];
                        sprintf((char *)ptemp, "[From:%d.%d.%d.%d:%d]:\r\n", oldaddr[0], oldaddr[1], oldaddr[2], oldaddr[3], oldport);  /* 准备输出新的目标IP地址和端口号 */
                        tempx = strlen((char *)rmemo->text) + strlen((char *)ptemp);
                        if (tempx > NET_RMEMO_MAXLEN)
                        {
                            rmemo->text[0] = 0;
                        }
                        strcat(((char *)rmemo->text), (char *)ptemp);
                    }
                    
                    tempx = strlen((char *)rmemo->text) + netdev.socket_recv_len[socket_index];
                    if (tempx > NET_RMEMO_MAXLEN)
                    {
                        rmemo->text[0] = 0;
                    }
                    netdev.socket_recv_buf[socket_index][netdev.socket_recv_len[socket_index]] = '\0';
                    strcat(((char *)rmemo->text), (char *)netdev.socket_recv_buf[socket_index]);
                    rxcnt += netdev.socket_recv_len[socket_index];
                    memo_draw_memo(rmemo, 1);
                    net_msg_show(ip_height, msg_height, fsize, txcnt, rxcnt, protocol, 1 << 1);                                         /* 更新提示信息 */
                    
                    netdev.socket_recv_status[socket_index] = 0;
                }
            }
        }
        
        if (oldconnstatus != connstatus)                                                                        /* 连接状态改变 */
        {
            oldconnstatus = connstatus;                                                                         /* 保存当前连接状态 */
            
            if (connstatus == 0)                                                                                /* 连接状态由连接变为断开 */
            {
                oldaddr[0] = 0;
                oldaddr[1] = 0;
                oldaddr[2] = 0;
                oldaddr[3] = 0;
                oldport = 0;
                for (socket_index=0; socket_index<NET_TCP_SERVER_MAX_CONN + 1; socket_index++)
                {
                    netdev.tcp_conn_status[socket_index] = 0;
                    netdev.socket_send_busy_status[socket_index] = 0;
                    netdev.socket_recv_status[socket_index] = 0;
                    netdev.socket_recv_len[socket_index] = 0;
                    socket.socket_index = socket_index;
                    socket.enable = CH395Q_DISABLE;                                                             /* 关闭Socket */
                    ch395q_socket_config(&socket);
                    gui_memex_free(netdev.socket_recv_buf[socket_index]);                                       /* 释放Socket的数据接收缓冲区 */
                }
                bkcolor = gui_memex_malloc(200 * 80 * 2);                                                       /* 显示连接断开提示信息 */
                if (bkcolor != NULL)
                {
                    app_read_bkcolor((lcddev.width - 200) / 2, (lcddev.height - 80) / 2, 200, 80, bkcolor);
                    window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 80) / 2,
                                   200, 80,
                                   (uint8_t *)netplay_connmsg_tbl[3][gui_phy.language],
                                   (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                                   12, 0, 0, 0);
                    delay_ms(800);
                    app_recover_bkcolor((lcddev.width - 200) / 2, (lcddev.height - 80) / 2, 200, 80, bkcolor);
                    gui_memex_free(bkcolor);
                }
                protbtn->sta = 0;                                                                               /* 激活协议选择按钮 */
                connbtn->caption = netplay_btncaption_tbl[1][gui_phy.language];                                 /* 更新连接按钮的文本 */
            }
            else                                                                                                /* 连接状态由断开变为连接 */
            {
                protbtn->sta = 2;                                                                               /* 设置协议选择按钮为非激活 */
                connbtn->caption = netplay_btncaption_tbl[2][gui_phy.language];                                 /* 更新连接按钮的文本 */
                editflag = 0;                                                                                   /* 只允许编辑发送区的输入框 */
                edit_show_cursor(eip, 0);                                                                       /* 关闭IP地址编辑框的光标 */
                edit_show_cursor(eport, 0);                                                                     /* 关闭端口号编辑框的光标 */
                eip->type = 0x04;                                                                               /* IP地址编辑框的光标不闪烁 */
                eport->type = 0x04;                                                                             /* 端口号编辑框的光标不闪烁 */
                smemo->type = 0x01;                                                                             /* 允许编辑发送区的输入框，闪烁并显示光标 */
            }
            
            btn_draw(protbtn);                                                                                  /* 重绘协议选择按钮 */
            btn_draw(connbtn);                                                                                  /* 重绘连接按钮 */
            net_edit_colorset(eip, eport, protocol, connstatus);                                                /* 重绘IP地址和端口号编辑框 */
        }
    }
    
    if(connstatus == 1)                                                                 /* 若已连接，则需先关闭连接 */
    {
        connstatus = 0;                                                                 /* 标记为未连接 */
        for (socket_index=0; socket_index<NET_TCP_SERVER_MAX_CONN + 1; socket_index++)
        {
            socket.socket_index = socket_index;
            socket.enable = CH395Q_DISABLE;                                             /* 关闭Socket */
            ch395q_socket_config(&socket);
            gui_memex_free(netdev.socket_recv_buf[socket_index]);                       /* 释放Socket数据接收缓冲区 */
        }
    }
    
    gui_memin_free(ptemp);  /* 释放内存并删除各个控件 */
    edit_delete(eip);
    edit_delete(eport);
    memo_delete(rmemo);
    memo_delete(smemo);
    t9_delete(t9);
    btn_delete(protbtn);
    btn_delete(connbtn);
    btn_delete(clrbtn);
    btn_delete(sendbtn);
    system_task_return = 0;
    
    return 0;
}

/**
 * @brief       网络通信测试
 * @param       无
 * @retval      无
 */
uint8_t net_play(void)
{
    uint8_t res;
    uint16_t yoff = 0;
    _btn_obj *tbtn = 0;
    uint8_t retry = 0;
    uint8_t i;
    
    if (lcddev.width == 240)    /* 根据LCD尺寸设置参数 */
    {
        yoff = 170;
    }
    else
    {
        yoff = 210;
    }
    
    tbtn = btn_creat((lcddev.width - 180) / 2, yoff + (lcddev.height - yoff - 60) / 2, 180, 60, 0, 0);      /* 创建标准按钮 */
    if (tbtn == NULL)
    {
        return 1;
    }
    
    if (audiodev.status & (1 << 7))                                                                         /* 判断当前是否正在播放音乐 */
    {
        audio_stop_req(&audiodev);                                                                          /* 停止音频播放 */
        audio_task_delete();                                                                                /* 删除音乐播放任务 */
    }
    
    window_msg_box((lcddev.width - 220) / 2, (lcddev.height - 100) / 2,                                     /* 显示正在初始化提示框 */
                   220, 100,
                   (uint8_t *)netplay_remindmsg_tbl[0][gui_phy.language],
                   (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                   12, 0, 0, 0);
    res = ch395q_init(1);                                                                                   /* 初始化CH395Q */
    if (res == 0)                                                                                           /* CH395Q初始化成功 */
    {
        ch395q_cmd_get_mac_addr(netdev.mac);
        ch395q_net_config(CH395Q_ENABLE, NULL, NULL, NULL, phy_conn_cb, phy_disconn_cb, dhcp_success_cb);   /* CH395Q网络配置，开启DHCP */
        ch395q_handler_task_creat();                                                                        /* 创建CH395Q处理任务 */
        
        window_msg_box((lcddev.width - 220) / 2, (lcddev.height - 100) / 2,                                 /* 显示等待DHCP提示框 */
                       220, 100,
                       (uint8_t *)netplay_remindmsg_tbl[2][gui_phy.language],
                       (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                       12, 0, 0, 0);
        retry = 100;
        while ((netdev.dhcp_status != 1) && (retry != 0))                                                   /* 等DHCP成功 */
        {
            delay_ms(100);
            retry--;
        }
        
        if (netdev.dhcp_status == 1)                                                                        /* DHCP成功 */
        {
            window_msg_box((lcddev.width - 220) / 2, (lcddev.height - 100) / 2,                             /* 显示DHCP成功提示框 */
                           220, 100,
                           (uint8_t *)netplay_remindmsg_tbl[3][gui_phy.language],
                           (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                           12, 0, 0, 0);
        }
        else                                                                                                /* DHCP失败 */
        {
            window_msg_box((lcddev.width - 220) / 2, (lcddev.height - 100) / 2,                             /* 显示DHCP失败提示框 */
                           220, 100,
                           (uint8_t *)netplay_remindmsg_tbl[4][gui_phy.language],
                           (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                           12, 0, 0, 0);
            ch395q_net_config(CH395Q_DISABLE,                                                               /* 重新进行CH395Q网络配置，关闭DHCP */
                              static_ip, static_gwip, static_mask,
                              phy_conn_cb, phy_disconn_cb, dhcp_success_cb);
            for (i=0; i<sizeof(netdev.ip); i++)
            {
                netdev.ip[i] = static_ip[i];
            }
            for (i=0; i<sizeof(netdev.gwip); i++)
            {
                netdev.gwip[i] = static_gwip[i];
            }
            for (i=0; i<sizeof(netdev.mask); i++)
            {
                netdev.mask[i] = static_mask[i];
            }
        }
        delay_ms(1000);
        tbtn->caption = netplay_tbtncaption_tb[gui_phy.language];                                           /* 按钮名称 */
        tbtn->font = 24;                                                                                    /* 按钮字体 */
        net_load_ui();                                                                                      /* 加载网路测试主界面 */
        btn_draw(tbtn);                                                                                     /* 绘制测试按钮 */
        
        while (1)
        {
            tp_dev.scan(0);                                                     /* 触摸屏扫描 */
            in_obj.get_key(&tp_dev, IN_TYPE_TOUCH);                             /* 获取触摸信息 */
            delay_ms(5);
            
            if (system_task_return)                                             /* 强制退出有效 */
            {
                delay_ms(15);
                if (tpad_scan(1))                                               /* 再次确认TPAD，排除干扰 */
                {
                    ch395q_handler_task_delete();                               /* 删除CH395Q处理任务 */
                    break;
                }
                else
                {
                    system_task_return = 0;                                     /* 清除强制退出标志 */
                }
            }
            
            res = btn_check(tbtn, &in_obj);                                     /* 检查开始测试按钮 */
            if (res && ((tbtn->sta & (1 << 7)) == 0) && (tbtn->sta & (1 << 6))) /* 开始测试按钮被按下并松开 */
            {
                net_test();                                                     /* 进入网络测试 */
                net_load_ui();                                                  /* 重新加载网路测试主界面 */
                btn_draw(tbtn);                                                 /* 重绘开始测试按钮 */
            }
        }
    }
    else                                                                        /* CH395Q初始化失败 */
    {
        window_msg_box((lcddev.width - 220) / 2, (lcddev.height - 100) / 2,     /* 显示初始化失败提示框 */
                       220, 100,
                       (uint8_t *)netplay_remindmsg_tbl[1][gui_phy.language],
                       (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language],
                       12, 0, 0, 0);
        delay_ms(2000);
    }
    
    ch395q_cmd_enter_sleep();                                                   /* CH395Q进入低功耗模式 */
    btn_delete(tbtn);
    system_task_return = 0;
    
    return 0;
}
