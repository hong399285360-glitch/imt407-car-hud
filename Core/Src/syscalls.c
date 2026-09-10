/**
  ******************************************************************************
  * @file   syscalls.c
  * @brief  newlib-nano 系统调用存根
  ******************************************************************************
  */

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <errno.h>
#include <time.h>
#include <stm32f4xx_hal.h>

/* 可重入性需要的 errno */
#undef errno
extern int errno;

/*
 * _sbrk - 增加程序数据空间（堆分配）
 */
caddr_t _sbrk(int incr)
{
    extern char _end;        /* 链接脚本定义的堆起始地址 */
    extern char _estack;     /* 栈顶地址 */
    static char *heap_end = &_end;
    char *prev_heap_end;

    prev_heap_end = heap_end;

    if (heap_end + incr > &_estack) {
        /* 堆和栈碰撞了 */
        errno = ENOMEM;
        return (caddr_t)-1;
    }

    heap_end += incr;
    return (caddr_t)prev_heap_end;
}

/*
 * _write - 写文件（stdout/stderr 重定向）
 *
 * 本板没有独立的调试串口：UART4(PA0/PA1) 已被 ESP8266 的 AT 协议独占，
 * 往里塞 printf 会污染 AT 指令流；USART1 未配置引脚。所以这里只做空实现。
 *
 * 这个函数必须存在且不能依赖未定义的符号：启用浮点 printf 后
 * （链接参数 -u _printf_float），newlib 的 vfprintf 会经 __sfvwrite_r
 * 引用 _write，缺了就链接失败。历史写法指向不存在的 huart1，之前因为
 * 工程里没人调用 printf、gc-sections 把 .text._write 整段丢掉才侥幸通过。
 *
 * 需要调试输出请用 SDLog_Log() 写 SD 卡日志。
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    return len;
}

/*
 * _read - 读文件
 */
int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/*
 * _close - 关闭文件
 */
int _close(int file)
{
    (void)file;
    return -1;
}

/*
 * _fstat - 文件状态
 */
int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

/*
 * _isatty - 是否是终端设备
 */
int _isatty(int file)
{
    (void)file;
    return 1;
}

/*
 * _lseek - 文件指针定位
 */
int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

/*
 * _kill - 终止进程
 */
void _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    while (1);
}

/*
 * _getpid - 获取进程 ID
 */
int _getpid(void)
{
    return 1;
}

/*
 * _exit - 退出程序
 */
void _exit(int status)
{
    (void)status;
    while (1);
}
