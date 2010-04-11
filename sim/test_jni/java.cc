#include <jni.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int sockfd;
struct sockaddr_in srcaddr;
jobject simpleserver;
jmethodID serveMthd;
JNIEnv *jni = 0;

void dump(char *data, int len)
{
    for (int i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");
}

void send_reply(char *data, int len)
{
    printf("sending reply:\n");
    dump(data, len);

    if (sendto(sockfd, (const void *)data, len, 0,
            (const sockaddr*)&srcaddr, sizeof(srcaddr)) == -1)
        perror("sendto");
}

void handle_packet(char *data, int len)
{
    printf("got packet:\n");
    dump(data, len);

    jbyteArray arr = jni->NewByteArray(len);
    jni->SetByteArrayRegion(arr, 0, len, (jbyte *)data);
    jbyteArray ret = (jbyteArray)jni->CallObjectMethod(simpleserver, serveMthd, arr);
    
    jboolean isCopy = false;
    jbyte *ret_data = jni->GetByteArrayElements(ret, &isCopy);

    send_reply((char *)ret_data, jni->GetArrayLength(ret));
    if (isCopy)
        free(ret_data);
}

int main()
{
    JavaVM *jvm = 0;
    JavaVMInitArgs args;
    JavaVMOption options[1];

    args.version = JNI_VERSION_1_4;
    args.nOptions = 1;
    args.options = options;
    options[0].optionString = (char *) "-Djava.class.path=../../bin/";

    int err = JNI_CreateJavaVM(&jvm, (void **)&jni, &args);

    jclass cls = jni->FindClass("server/SimpleServer");
    jmethodID constructor = jni->GetMethodID(cls, "<init>", "(I)V");
    simpleserver = jni->NewObject(cls, constructor, 4*1024*1024);

    serveMthd = jni->GetMethodID(cls, "serve", "([B)[B");

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(15712);

    if (bind(sockfd, (const sockaddr*)&sin, sizeof(sin)) < 0)
    {
        perror("bind");
        return 2;
    }

    while (1)
    {
        char buf[2048];

        socklen_t addrlen = sizeof(srcaddr);
        ssize_t size = recvfrom(sockfd, (void *)buf, sizeof(buf), 0, (sockaddr *)&srcaddr, &addrlen);
        if (size == -1)
        {
            perror("recvfrom");
            return 3;
        }

        handle_packet(buf, size);
    }
}
