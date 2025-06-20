#include <reactor_server/net/buffer.h>
#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <algorithm>

using namespace rs_buffer;

void testBasicConstruction()
{
    std::cout << "测试基本构造..." << std::endl;

    Buffer buf;
    assert(buf.getReadableSize() == 0);
    assert(buf.getBackWritableSize() == 1024); // 默认大小
    assert(buf.getFrontWritableSize() == 0);

    std::cout << "✓ 基本构造测试通过" << std::endl;
}

void testWriteOperations()
{
    std::cout << "测试写入操作..." << std::endl;

    Buffer buf;
    const char *testData = "Hello World";
    size_t dataLen = strlen(testData);

    // 测试 write_noMove (不移动写指针)
    uint64_t initialWritable = buf.getBackWritableSize();
    buf.write_noMove((void *)testData, dataLen);
    assert(buf.getReadableSize() == 0); // 写指针未移动，所以可读大小为0
    assert(buf.getBackWritableSize() == initialWritable);

    // 测试 write_move (移动写指针)
    buf.write_move((void *)testData, dataLen);
    assert(buf.getReadableSize() == dataLen);
    assert(buf.getBackWritableSize() == 1024 - dataLen);

    std::cout << "✓ 写入操作测试通过" << std::endl;
}

void testReadOperations()
{
    std::cout << "测试读取操作..." << std::endl;

    Buffer buf;
    const char *testData = "Hello World";
    size_t dataLen = strlen(testData);

    // 先写入数据
    buf.write_move((void *)testData, dataLen);

    // 测试 read_noMove (不移动读指针)
    char readBuffer[20] = {0};
    buf.read_noMove(readBuffer, dataLen);
    assert(strcmp(readBuffer, testData) == 0);
    assert(buf.getReadableSize() == dataLen); // 读指针未移动

    // 测试 read_move (移动读指针)
    memset(readBuffer, 0, sizeof(readBuffer));
    buf.read_move(readBuffer, 5); // 读取 "Hello"
    assert(strncmp(readBuffer, "Hello", 5) == 0);
    assert(buf.getReadableSize() == dataLen - 5); // 剩余可读数据

    std::cout << "✓ 读取操作测试通过" << std::endl;
}

void testStringOperations()
{
    std::cout << "测试字符串操作..." << std::endl;

    Buffer buf;
    std::string testStr = "Hello String";

    // 测试字符串写入
    buf.write_move(testStr, testStr.length());
    assert(buf.getReadableSize() == testStr.length());

    // 测试字符串读取 (需要先调整字符串大小)
    std::string readStr;
    readStr.resize(testStr.length());
    buf.read_move(readStr, testStr.length());
    readStr.resize(testStr.length()); // 确保正确的大小
    assert(readStr == testStr);

    std::cout << "✓ 字符串操作测试通过" << std::endl;
}

void testBufferToBuffer()
{
    std::cout << "测试缓冲区间操作..." << std::endl;

    Buffer buf1, buf2;
    const char *testData = "Buffer Test";
    size_t dataLen = strlen(testData);

    // 向buf1写入数据
    buf1.write_move((void *)testData, dataLen);

    // 从buf1复制到buf2
    buf2.write_move(buf1);

    // 验证buf2中的数据
    char readBuffer[20] = {0};
    buf2.read_move(readBuffer, dataLen);
    assert(strcmp(readBuffer, testData) == 0);

    std::cout << "✓ 缓冲区间操作测试通过" << std::endl;
}

void testLineReading()
{
    std::cout << "测试行读取..." << std::endl;

    Buffer buf;

    // 测试 \r\n 结尾
    const char *testLine1 = "Line 1\r\nLine 2\n";
    buf.write_move((void *)testLine1, strlen(testLine1));

    std::string line;
    buf.readLine_move(line);
    assert(line == "Line 1\r\n");

    // 测试 \n 结尾
    buf.readLine_move(line);
    assert(line == "Line 2\n");

    std::cout << "✓ 行读取测试通过" << std::endl;
}

void testSpaceManagement()
{
    std::cout << "测试空间管理..." << std::endl;

    Buffer buf;

    // 写入数据然后读取一部分，创建前置空间
    const char *testData = "0123456789";
    buf.write_move((void *)testData, 10);

    char readBuffer[5];
    buf.read_move(readBuffer, 5); // 读取前5个字符

    assert(buf.getFrontWritableSize() == 5);        // 前置可写空间
    assert(buf.getReadableSize() == 5);             // 剩余可读数据
    assert(buf.getBackWritableSize() == 1024 - 10); // 后置可写空间

    std::cout << "✓ 空间管理测试通过" << std::endl;
}

void testPointerMovement()
{
    std::cout << "测试指针移动..." << std::endl;

    Buffer buf;
    const char *testData = "Test Data";
    buf.write_move((void *)testData, 9);

    // 测试读指针移动
    uint64_t initialReadable = buf.getReadableSize();
    buf.moveReadPtr(4);
    assert(buf.getReadableSize() == initialReadable - 4);

    // 验证读指针位置正确
    char readBuffer[6] = {0};
    buf.read_noMove(readBuffer, 5);
    assert(strcmp(readBuffer, " Data") == 0);

    std::cout << "✓ 指针移动测试通过" << std::endl;
}

void testClearOperation()
{
    std::cout << "测试清理操作..." << std::endl;

    Buffer buf;
    const char *testData = "Some data";
    buf.write_move((void *)testData, strlen(testData));
    buf.read_move(nullptr, 4); // 移动读指针

    // 清理前状态检查
    assert(buf.getReadableSize() > 0);
    assert(buf.getFrontWritableSize() > 0);

    // 清理操作
    buf.clear();

    // 清理后状态检查
    assert(buf.getReadableSize() == 0);

    std::cout << "✓ 清理操作测试通过" << std::endl;
}

void testEdgeCases()
{
    std::cout << "测试边界情况..." << std::endl;

    Buffer buf;

    // 测试空缓冲区读取
    std::string emptyLine;
    buf.readLine_noMove(emptyLine);
    assert(emptyLine.empty());

    // 测试大数据写入 (触发扩容)
    std::string largeData(2000, 'X'); // 超过默认1024大小
    buf.write_move(largeData, largeData.length());
    assert(buf.getReadableSize() == 2000);

    std::cout << "✓ 边界情况测试通过" << std::endl;
}

int main()
{
    std::cout << "开始 Buffer 类功能测试...\n"
              << std::endl;

    try
    {
        testBasicConstruction();
        testWriteOperations();
        testReadOperations();
        testStringOperations();
        testBufferToBuffer();
        testLineReading();
        testSpaceManagement();
        testPointerMovement();
        testClearOperation();
        testEdgeCases();

        std::cout << "\n🎉 所有测试通过！" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}