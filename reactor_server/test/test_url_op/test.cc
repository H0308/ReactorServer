#include <iostream>
#include <reactor_server/net/http/utils/url_op.h>

int main()
{
    std::string str = "C  ";
    std::string out1;
    bool ret = rs_url_op::UrlOp::urlEncode(out1, str, true);
    if(ret)
        std::cout << out1 << std::endl;
    
    std::string out2;
    ret = rs_url_op::UrlOp::urlDecode(out2, out1, true);
    if (ret)
        std::cout << out2 << std::endl;

    return 0;
}