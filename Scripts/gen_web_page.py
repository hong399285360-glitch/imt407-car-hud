#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 WebUI/index.html 转成 C 字节数组, 供 F407 内嵌下发 HUD 页面。

改了 WebUI/index.html 之后必须重新运行本脚本, 否则烧进板子的还是旧页面。

用法:  python Scripts/gen_web_page.py
输出:  Core/Src/web_page.c
       Core/Inc/web_page.h
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "WebUI", "index.html")
OUT_C = os.path.join(ROOT, "Core", "Src", "web_page.c")
OUT_H = os.path.join(ROOT, "Core", "Inc", "web_page.h")

HEADER_H = """#ifndef WEB_PAGE_H
#define WEB_PAGE_H

/* 内嵌 HUD 前端页面 (源文件 WebUI/index.html, 由 Scripts/gen_web_page.py 生成)
   改完 index.html 请重新运行: python Scripts/gen_web_page.py */
extern const char web_page_html[];
extern const unsigned int web_page_html_len;

#endif
"""


def main():
    if not os.path.isfile(SRC):
        print("找不到源文件: %s" % SRC)
        return 1

    with open(SRC, "rb") as f:
        data = f.read()
    n = len(data)
    if n == 0:
        print("源文件为空: %s" % SRC)
        return 1

    with open(OUT_H, "w", encoding="utf-8", newline="\n") as f:
        f.write(HEADER_H)

    with open(OUT_C, "w", encoding="utf-8", newline="\n") as f:
        f.write('#include "web_page.h"\n\n')
        f.write("/* 自动生成, 请勿手工编辑。源文件: WebUI/index.html (%d 字节) */\n" % n)
        f.write("const char web_page_html[] = {\n")
        for i in range(0, n, 16):
            chunk = data[i:i + 16]
            f.write("    " + " ".join("0x%02x," % b for b in chunk) + "\n")
        f.write("};\n\n")
        f.write("const unsigned int web_page_html_len = %d;\n" % n)

    print("生成完成: %d 字节" % n)
    print("  %s" % OUT_C)
    print("  %s" % OUT_H)
    return 0


if __name__ == "__main__":
    sys.exit(main())
