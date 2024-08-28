#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
*/

class UserService:public fixbug::UserServiceRpc  //使用在rpc服务发布端（rpc服务提供者）
{
public:
    bool Login(std::string name,std::string pwd)
    {
        std::cout<<"doing local service:Login"<<std::endl;
        std::cout<<"name:"<<name<<"pwd"<<pwd<<std::endl;
        return true;
    }

    bool Register(uint32_t id,std::string name,std::string pwd)
    {
        std::cout<<"doing local service:Register"<<std::endl;
        std::cout<<"id"<<id<<"name:"<<name<<"pwd"<<pwd<<std::endl;
        return true;
    }

    //这段代码作用，站在服务提供者的角度上，你（rpc服务调用者）想调用我的login方法（即上面本地的Login方法），首先你会发送一个rpc请求到我这来，然后我的rpc框架接收，
    //rpc框架根据你的请求，查看你请求的是哪个方法，都有哪些参数，需要返回什么，然后rpc框架帮我们匹配到底下的login方法，然后框架就会帮我们把网络上的请求上报上来，
    //我们接收到请求，从请求中拿出数据然后做本地业务，然后填相应的响应，然后再执行一个回调，相当于把执行完的rpc方法的返回值再塞给框架，由框架给我们做数据的序列化
    //然后再通过框架的网络把响应返回回去，返回给你（rpc服务调用者）
    /*
    重写基类UserServiceRpc的虚函数 下面这些方法都是框架直接调用的
    1. caller    ====》    Login(LoginRequest) returns(LoginResponse)  =>muduo ==>  callee
    2. callee    ===>      Login(LoginRequest)   =>交到下面重写的这个login方法上了
    */
    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done)
    {
        //框架给业务上报了请求参数 LoginRequest，应用获取相应数据做本地业务
        std::string name=request->name();
        std::string pwd=request->pwd();

        //做本地业务
        bool login_result=Login(name,pwd); 

        //把响应写入   包括错误码、错误消息、返回值
        fixbug::ResultCode *code=response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //执行回调操作    执行响应对象数据的序列化和网络发送（都是由框架完成的）
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                        const ::fixbug::RegisterRequest* request,
                        ::fixbug::RegisterResponse* response,
                        ::google::protobuf::Closure* done)
    {
        uint32_t id=request->id();
        std::string name=request->name();
        std::string pwd=request->pwd();

        bool ret=Register(id,name,pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }

};

int main(int argc,char **argv)
{
    //调用框架的初始化操作
    MprpcApplication::Init(argc,argv);

    //把UserService对象发布到rpc节点上    provider是一个rpc网络服务对象
    RpcProvider provider;
    provider.NotifyService(new UserService());

    //启动一个rpc服务发布节点    Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}