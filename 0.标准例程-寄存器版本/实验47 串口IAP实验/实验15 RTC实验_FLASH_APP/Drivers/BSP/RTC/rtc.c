/**
 ****************************************************************************************************
 * @file        rtc.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-22
 * @brief       RTC 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20200422
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/RTC/rtc.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"


_calendar_obj calendar;     /* 时钟结构体 */

/**
 * @brief       RTC写入后备区域SRAM
 * @param       bkrx : 后备区寄存器编号,范围:0~41
 * @param       data : 要写入的数据,16位长度
 * @retval      无
 */
void rtc_write_bkr(uint32_t bkrx, uint16_t data)
{
    uint32_t temp = 0;
    PWR->CR |= 1 << 8;  /* 取消备份区写保护 */
    temp = BKP_BASE + 4 + bkrx * 4;
    (*(uint16_t *)temp) = data;
}

/**
 * @brief       RTC读取后备区域SRAM
 * @param       bkrx : 后备区寄存器编号,范围:0~41
 * @retval      读取到的值
 */
uint16_t rtc_read_bkr(uint32_t bkrx)
{
    uint32_t temp = 0;
    temp = BKP_BASE + 4 + bkrx * 4;
    return (*(uint16_t *)temp); /* 返回读取到的值 */
}

/**
 * @brief       RTC初始化
 *   @note
 *              默认尝试使用LSE,当LSE启动失败后,切换为LSI.
 *              通过BKP寄存器0的值,可以判断RTC使用的是LSE/LSI:
 *              当BKP0==0X5050时,使用的是LSE
 *              当BKP0==0X5051时,使用的是LSI
 *              注意:切换LSI/LSE将导致时间/日期丢失,切换后需重新设置.
 *
 * @param       无
 * @retval      0,成功
 *              1,进入初始化模式失败
 */
uint8_t rtc_init(void)
{
    /* 检查是不是第一次配置时钟 */
    uint16_t bkpflag = 0;
    uint16_t retry = 200;
    uint32_t tempreg = 0;
    uint32_t clockfreq = 0;

    RCC->APB1ENR |= 1 << 28;        /* 使能电源时钟 */
    RCC->APB1ENR |= 1 << 27;        /* 使能备份时钟 */
    PWR->CR |= 1 << 8;              /* 取消备份区写保护 */

    bkpflag = rtc_read_bkr(0);      /* 读取BKP0的值 */

    if (bkpflag != 0X5050)          /* 之前使用的不是LSE */
    {
        RCC->BDCR |= 1 << 0;        /* 开启外部低速振荡器 */
        
        while (retry && ((RCC->BDCR & 0X02) == 0))  /* 等待LSE准备好 */
        {
            retry--;
            delay_ms(5);
        }
        
        tempreg = RCC->BDCR;        /* 读取BDCR的值 */
        tempreg &= ~(3 << 8);       /* 清零8/9位 */

        if (retry == 0)             /* 开启LSE失败? */
        {
            RCC->CSR |= 1 << 0;         /* LSI使能 */
        
            while (!(RCC->CSR & 0x02)); /* 等待LSI就绪 */
            
            tempreg |= 1 << 9;          /* LSE开启失败,启动LSI. */
            clockfreq = 40000 - 1;      /* LSI频率约40Khz(参考F103数据手册说明) */
            rtc_write_bkr(0, 0X5051);   /* 标记已经初始化过了,使用LSI */
        }
        else
        {
            tempreg |= 1 << 8;          /* 选择LSE,作为RTC时钟 */
            clockfreq = 32768 - 1;      /* LSE频率为32.769Khz */
            rtc_write_bkr(0, 0X5050);   /* 标记已经初始化过了,使用LSE */
        }

        tempreg |= 1 << 15;         /* 使能RTC时钟 */
        RCC->BDCR = tempreg;        /* 重新设置BDCR寄存器 */

        while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成 */

        while (!(RTC->CRL & (1 << 3))); /* 等待RTC寄存器同步 */

        RTC->CRH |= 1 << 0;             /* SECF = 1, 允许秒中断 */
        RTC->CRH |= 1 << 1;             /* ALRF = 1, 允许闹钟中断 */

        while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成 */

        RTC->CRL |= 1 << 4;         /* 允许配置 */
        RTC->PRLH = 0X0000;
        RTC->PRLL = clockfreq;      /* 时钟周期设置(有待观察,看是否跑慢了?)理论值：32767 */
        RTC->CRL &= ~(1 << 4);      /* 配置更新 */

        while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成 */

        if (bkpflag != 0X5051)      /* BKP0的内容既不是0X5050,也不是0X5051,说明是第一次配置,需要设置时间日期. */
        {
            rtc_set_time(2020, 4, 22, 6, 59, 55);   /* 设置时间 */
        }
    }
    else     /* 系统继续计时 */
    {
        retry = 30;     /* 避免卡死 */
        
        while ((!(RTC->CRL & (1 << 3)) && retry))   /* 等待RTC寄存器同步 */
        {
            delay_ms(5);
            retry--;
        }

        retry = 100;    /* 检测LSI/LSE是否正常工作 */
        
        tempreg = RTC->DIVL;            /* 读取DIVL寄存器的值 */
        while (retry)
        {
            delay_ms(5);
            retry--;
            
            if (tempreg != RTC->DIVL)   /* 对比DIVL和tempreg, 如果有差异, 则退出 */
            {
                break;                  /* DIVL != tempreg, 说明RTC在计数, 说明晶振没问题 */
            }
        }
        if (retry == 0)
        {
            rtc_write_bkr(0, 0XFFFF);   /* 标记错误的值 */
            RCC->BDCR = 1 << 16;        /* 复位BDCR */
            delay_ms(10);
            RCC->BDCR = 0;              /* 结束复位 */
            return 1;                   /* 初始化失败 */
        }
        else
        {
            RTC->CRH |= 0X01;           /* 允许秒中断 */

            while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成 */
        }
    }

    sys_nvic_init(0, 0, RTC_IRQn, 2);   /* 优先级设置 */
    rtc_get_time();  /* 更新时间 */
    return 0;
}

/**
 * @brief       RTC时钟中断
 *   @note      秒钟中断 / 闹钟中断 共用同一个中断服务函数
 *              根据RTC_CRL寄存器的 SECF 和 LARF 位区分是哪个中断
 * @param       无
 * @retval      无
 */
void RTC_IRQHandler(void)
{
    rtc_get_time();             /* 更新时间 */

    if (RTC->CRL & (1 << 0))    /* SECF = 1, 秒钟中断 */
    {
        RTC->CRL &= ~(1 << 0);  /* SECF = 0, 清秒钟中断 */
        //printf("sec:%d\r\n", calendar.sec);   /* 打印秒钟 */
    }

    if (RTC->CRL & (1 << 1))    /* ALRF = 1, 闹钟中断 */
    {
        RTC->CRL &= ~(1 << 1);  /* ALRF = 0, 清闹钟中断 */

        /* 输出闹铃时间 */
        printf("Alarm Time:%d-%d-%d %d:%d:%d\n", calendar.year, calendar.month, calendar.date, calendar.hour, calendar.min, calendar.sec);
    }

    RTC->CRL &= ~(1 << 2);      /* OWF = 0, 清除溢出中断标志 */

    while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成, 即等待RTOFF == 1 */
}

/**
 * @brief       判断年份是否是闰年
 *   @note      月份天数表:
 *              月份   1  2  3  4  5  6  7  8  9  10 11 12
 *              闰年   31 29 31 30 31 30 31 31 30 31 30 31
 *              非闰年 31 28 31 30 31 30 31 31 30 31 30 31
 * @param       year : 年份
 * @retval      0, 非闰年; 1, 是闰年;
 */
static uint8_t rtc_is_leap_year(uint16_t year)
{
    /* 闰年规则: 四年闰百年不闰，四百年又闰 */
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief       设置时间, 包括年月日时分秒
 *   @note      以1970年1月1日为基准, 往后累加时间
 *              合法年份范围为: 1970 ~ 2105年
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @param       hour  : 小时
 * @param       min   : 分钟
 * @param       sec   : 秒钟
 * @retval      0, 成功; 1, 失败;
 */
uint8_t rtc_set_time(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
    uint32_t seccount = 0;

    seccount = rtc_date2sec(syear, smon, sday, hour, min, sec); /* 将年月日时分秒转换成总秒钟数 */

    /* 设置时钟 */
    RCC->APB1ENR |= 1 << 28;    /* 使能电源时钟 */
    RCC->APB1ENR |= 1 << 27;    /* 使能备份时钟 */
    PWR->CR |= 1 << 8;          /* 取消备份区写保护 */
    /* 上面三步是必须的! */
    RTC->CRL |= 1 << 4;         /* 允许配置 */
    RTC->CNTL = seccount & 0xffff;
    RTC->CNTH = seccount >> 16;
    RTC->CRL &= ~(1 << 4);      /* 配置更新 */

    while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成 */

    rtc_get_time();             /* 设置完之后更新一下数据 */
    return 0;
}

/**
 * @brief       设置闹钟, 具体到年月日时分秒
 *   @note      以1970年1月1日为基准, 往后累加时间
 *              合法年份范围为: 1970 ~ 2105年
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @param       hour  : 小时
 * @param       min   : 分钟
 * @param       sec   : 秒钟
 * @retval      0, 成功; 1, 失败;
 */
uint8_t rtc_set_alarm(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
    uint32_t seccount = 0;

    seccount = rtc_date2sec(syear, smon, sday, hour, min, sec); /* 将年月日时分秒转换成总秒钟数 */

    /* 设置时钟 */
    RCC->APB1ENR |= 1 << 28;    /* 使能电源时钟 */
    RCC->APB1ENR |= 1 << 27;    /* 使能备份时钟 */
    PWR->CR |= 1 << 8;          /* 取消备份区写保护 */
    /* 上面三步是必须的! */
    RTC->CRL |= 1 << 4;         /* 允许配置 */
    RTC->ALRL = seccount & 0xffff;
    RTC->ALRH = seccount >> 16;
    RTC->CRL &= ~(1 << 4);      /* 配置更新 */

    while (!(RTC->CRL & (1 << 5))); /* 等待RTC寄存器操作完成 */

    return 0;
}

/**
 * @brief       得到当前的时间
 *   @note      该函数不直接返回时间, 时间数据保存在calendar结构体里面
 * @param       无
 * @retval      无
 */
void rtc_get_time(void)
{
    static uint16_t daycnt = 0;
    uint32_t seccount = 0;
    uint32_t temp = 0;
    uint16_t temp1 = 0;
    const uint8_t month_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};   /* 平年的月份日期表 */

    seccount = RTC->CNTH;       /* 得到计数器中的值(秒钟数) */
    seccount <<= 16;
    seccount += RTC->CNTL;

    temp = seccount / 86400;    /* 得到天数(秒钟数对应的) */

    if (daycnt != temp)         /* 超过一天了 */
    {
        daycnt = temp;
        temp1 = 1970;           /* 从1970年开始 */

        while (temp >= 365)
        {
            if (rtc_is_leap_year(temp1))    /* 是闰年 */
            {
                if (temp >= 366)
                {
                    temp -= 366;    /* 闰年的秒钟数 */
                }
                else
                {
                    break;
                }
            }
            else
            {
                temp -= 365;    /* 平年 */
            }
            
            temp1++;
        }

        calendar.year = temp1;  /* 得到年份 */
        temp1 = 0;

        while (temp >= 28)      /* 超过了一个月 */
        {
            if (rtc_is_leap_year(calendar.year) && temp1 == 1)  /* 当年是不是闰年/2月份 */
            {
                if (temp >= 29)
                {
                    temp -= 29; /* 闰年的秒钟数 */
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (temp >= month_table[temp1])
                {
                    temp -= month_table[temp1]; /* 平年 */
                }
                else
                {
                    break;
                }
            }

            temp1++;
        }

        calendar.month = temp1 + 1;     /* 得到月份 */
        calendar.date = temp + 1;       /* 得到日期 */
    }

    temp = seccount % 86400;            /* 得到秒钟数 */
    calendar.hour = temp / 3600;        /* 小时 */
    calendar.min = (temp % 3600) / 60;  /* 分钟 */
    calendar.sec = (temp % 3600) % 60;  /* 秒钟 */
    calendar.week = rtc_get_week(calendar.year, calendar.month, calendar.date); /* 获取星期 */
}

/**
 * @brief       将年月日时分秒转换成秒钟数
 *   @note      输入公历日期得到星期(起始时间为: 公元0年3月1日开始, 输入往后的任何日期, 都可以获取正确的星期)
 *              使用 基姆拉尔森计算公式 计算, 原理说明见此贴:
 *              https://www.cnblogs.com/fengbohello/p/3264300.html
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @retval      0, 星期天; 1 ~ 6: 星期一 ~ 星期六
 */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint8_t week = 0;

    if (month < 3)
    {
        month += 12;
        --year;
    }

    week = (day + 1 + 2 * month + 3 * (month + 1) / 5 + year + (year >> 2) - year / 100 + year / 400) % 7;
    return week;
}

/**
 * @brief       将年月日时分秒转换成秒钟数
 *   @note      以1970年1月1日为基准, 1970年1月1日, 0时0分0秒, 表示第0秒钟
 *              最大表示到2105年, 因为uint32_t最大表示136年的秒钟数(不包括闰年)!
 *              本代码参考只linux mktime函数, 原理说明见此贴:
 *              http://www.openedv.com/thread-63389-1-1.html
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @param       hour  : 小时
 * @param       min   : 分钟
 * @param       sec   : 秒钟
 * @retval      转换后的秒钟数
 */
static long rtc_date2sec(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
    uint32_t Y, M, D, X, T;
    signed char monx = smon;/* 将月份转换成带符号的值, 方便后面运算 */

    if (0 >=  (monx -= 2))  /* 1..12 -> 11,12,1..10 */
    { 
        monx += 12;         /* Puts Feb last since it has leap day */
        syear -= 1;
    }
    
    Y = (syear - 1) * 365 + syear / 4 - syear / 100 + syear / 400;  /* 公元元年1到现在的闰年数 */
    M = 367 * monx / 12 - 30 + 59;
    D = sday - 1;
    X = Y + M + D - 719162;                         /* 减去公元元年到1970年的天数 */
    T = ((X * 24 + hour) * 60 + min) * 60 + sec;    /* 总秒钟数 */
    return T;
}















