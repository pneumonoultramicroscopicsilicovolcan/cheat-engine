/*
 * ceservertest.c
 *
 *  Created on: Jul 1, 2013
 *      Author: eric
 *
 *  this is basically a standalone client process, but because i'm lazy it runs in the same context as the server
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef __ANDROID__
#include <sys/ptrace.h>
#endif
#include <asm/ptrace.h>
#include <linux/types.h>



#include "ceserver.h"
#include "api.h" //for debugevent

int pHandle;

int cenet_connect(void)
{
  int fd;
  int i;


  struct sockaddr_in addr;

  fd=socket(AF_INET, SOCK_STREAM, 0);


  memset(&addr, 0, sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(PORT);
  addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK); //0x1600a8c0; //INADDR_LOOPBACK;

  debug_log("calling connect... (port=%d)\n", PORT);
  i=connect(fd, (struct sockaddr *)&addr, sizeof(addr));

  debug_log("after connect. %d\n", i);


  return fd;
}

int cenet_OpenProcess(int fd, int pid)
{
#pragma pack(1)
  struct
  {
    char command;
    int pid;
  } op;
#pragma pack()

  int pHandle;

  debug_log("cenet_OpenProcess(%d,%d)\n", fd, pid);



  op.command=CMD_OPENPROCESS;
  op.pid=pid;

  pHandle=0;

  sendall(fd, &op, sizeof(op), 0);
  recv(fd, &pHandle, sizeof(pHandle),MSG_WAITALL);

  return pHandle;
}

unsigned char cenet_getArchitecture(int fd, int pHandle)
{
#pragma pack(1)
  struct
  {
    char command;
    HANDLE pHandle;
  } sd;
#pragma pack()
  unsigned char result;

  sd.command=CMD_GETARCHITECTURE;
  sd.pHandle=pHandle;

  sendall(fd, &sd, sizeof(sd), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);

  return result;
}

int cenet_startDebugger(int fd, int pHandle)
{
#pragma pack(1)
  struct
  {
    char command;
    HANDLE pHandle;
  } sd;
#pragma pack()

  int result;


  sd.command=CMD_STARTDEBUG;
  sd.pHandle=pHandle;

  sendall(fd, &sd, sizeof(sd), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);

  return result;

}

int cenet_waitForDebugEvent(int fd, int pHandle, DebugEvent* devent, int timeout)
{
#pragma pack(1)
  struct
  {
    char command;
    HANDLE pHandle;
    int timeout;
  } wfd;
#pragma pack()


  int result;




  wfd.command=CMD_WAITFORDEBUGEVENT;
  wfd.pHandle=pHandle;
  wfd.timeout=timeout;

  sendall(fd, &wfd, sizeof(wfd), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);
  if (result)
    recv(fd, devent, sizeof(DebugEvent), MSG_WAITALL);

  debug_log(">>>>>>>>>>>>>>>>>>cenet_waitForDebugEvent returned<<<<<<<<<<<<<<<<\n");

  return result;

}

BOOL cenet_getThreadContext(int fd, int pHandle, int tid, void* context)
{
#pragma pack(1)
  struct
  {
    char command;
    HANDLE hProcess;
    int tid;
    int type;
  } gtc;
#pragma pack()

  uint32_t result;


  debug_log("ceservertest: cenet_getThreadContext(%d, %d, %d, %p)\n", fd, pHandle, tid, context);

  gtc.command=CMD_GETTHREADCONTEXT;
  gtc.hProcess=pHandle;
  gtc.tid=tid;
  gtc.type=0;

  sendall(fd, &gtc, sizeof(gtc), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);

  debug_log("ceservertest cenet_getThreadContext returned %d\n", result);

  if (result)
  {
    CONTEXT c;
    uint32_t structsize;
    recv(fd, &structsize, sizeof(uint32_t), MSG_WAITALL);

    debug_log("structsize=%d\n", structsize);


    if (structsize<=sizeof(c))
    {
      recv(fd, &c, structsize, MSG_WAITALL);

      if (c.type==2)
        debug_log("ARM32 context type\n");

      if (c.type==3)
        debug_log("ARM64 context type\n");

      memcpy(context, &c, sizeof(c));
    }
    else
      debug_log("Received context is too big\n");
  }

  return result;

}

int cenet_continueFromDebugEvent(int fd, int pHandle, int tid, int ignore)
{
#pragma pack(1)
  struct
  {
    char command;
    HANDLE pHandle;
    int tid;
    int ignore;
  } cfd;
#pragma pack()

  int result;


  cfd.command=CMD_CONTINUEFROMDEBUGEVENT;
  cfd.pHandle=pHandle;
  cfd.tid=tid;
  cfd.ignore=ignore;

  sendall(fd, &cfd, sizeof(cfd), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);

  return result;
}

int cenet_readProcessMemory(int fd, int pHandle, unsigned long long address, void *dest, int size)
{
#pragma pack(1)
  struct
  {
    char command;
    uint32_t pHandle;
    uint64_t address;
    uint32_t size;
    uint8_t compress;

  } rpm;
#pragma pack()

  int result;

 // printf("cenet_readProcessMemory\n");


 // debug_log("cenet_readProcessMemory(%d, %d, %llx, %p, %d)", fd, pHandle, address, dest, size);
 // fflush(stdout);


  rpm.command=CMD_READPROCESSMEMORY;
  rpm.pHandle=pHandle;
  rpm.address=address;
  rpm.size=size;
  rpm.compress=0;

  sendall(fd, &rpm, sizeof(rpm), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);

 // debug_log("result=%d\n", result);
  recv(fd, dest, result, MSG_WAITALL);


  return result;

}

int cenet_setBreakpoint(int fd, int pHandle, int tid, void *Address, int bptype, int bpsize, int debugreg)
{
#pragma pack(1)
    struct
    {
      char command;
      HANDLE hProcess;
      int tid;
      int debugreg;
      uint64_t Address;
      int bptype;
      int bpsize;
    } sb;
#pragma pack()
    int result;

  debug_log("cenet_setBreakpoint sizeof(sb)=%d\n", sizeof(sb));
  sb.command=CMD_SETBREAKPOINT;
  sb.hProcess=pHandle;
  sb.tid=tid;
  sb.debugreg=debugreg;
  sb.Address=(uintptr_t)Address;
  sb.bptype=bptype;
  sb.bpsize=bpsize;

  sendall(fd, &sb, sizeof(sb), 0);


  recv(fd, &result, sizeof(result), MSG_WAITALL);

  return result;
}

int cenet_removeBreakpoint(int fd, int pHandle, int tid, int debugreg, int waswatchpoint)
{
#pragma pack(1)
    struct
    {
      char command;
      HANDLE hProcess;
      uint32_t tid;
      uint32_t debugreg;
      uint32_t waswatchpoint;
    } rb;
#pragma pack()
    int result;

  debug_log("cenet_removeBreakpoint\n");
  rb.command=CMD_REMOVEBREAKPOINT;
  rb.hProcess=pHandle;
  rb.tid=tid;
  rb.debugreg=debugreg;
  rb.waswatchpoint=waswatchpoint;

  sendall(fd, &rb, sizeof(rb), 0);
  recv(fd, &result, sizeof(result), MSG_WAITALL);

  return result;
}

int cenet_loadExtension(int fd, int pHandle)
{
#pragma pack(1)
    struct
    {
      char command;
      HANDLE hProcess;
    } le;
#pragma pack()
  int result;

  le.command=CMD_LOADEXTENSION;
  le.hProcess=pHandle;
  sendall(fd, &le, sizeof(le), 0);

  recv(fd, &result, sizeof(result),MSG_WAITALL);
  return result;
}


#define ARM_DBG_READ(N, M, OP2, VAL) do {\
         asm volatile("mrc p14, 0, %0, " #N "," #M ", " #OP2 : "=r" (VAL));\
} while (0)


int cenet_VirtualQueryExFull(int fd, int pHandle, DWORD flags)
{
#pragma pack(1)
    struct
    {
      char command;
      HANDLE hProcess;
      uint8_t flags;
    } vqef;
#pragma pack()

    vqef.command=CMD_VIRTUALQUERYEXFULL;
    vqef.hProcess=pHandle;
    vqef.flags=flags;

    sendall(fd, &vqef, sizeof(vqef),0);
}

void *CESERVERTEST_DEBUGGERTHREAD(void *arg)
{
  int count=0;
  int fd=cenet_connect();

  int dscr=0;
#ifdef __arm__
  ARM_DBG_READ(c0, c1, 0, dscr);
  debug_log("after: %x\n", dscr);
#endif




  debug_log("calling cenet_startDebugger\n");

  if (cenet_startDebugger(fd, pHandle))
  {
    int i;
    DebugEvent devent;

    debug_log("cenet_startDebugger=true\n");

    while (1)
    {
      count++;
      debug_log("count=%d\n", count);

      if (count==4)
      {
        debug_log("going to set breakpoint\n");

        i=cenet_setBreakpoint(fd, pHandle, -1, 0x00ce0000, 3, 4,0);
        //cenet_setBreakpoint(fd, pHandle, -1, 0x85e4, 0, 1,2);
        //cenet_setBreakpoint(fd, pHandle, -1, 0x85e4, 0, 1,3);
       // cenet_setBreakpoint(fd, pHandle, -1, 0x85e4, 0, 1,4);

        //cenet_setBreakpoint(fd, pHandle, -1, 0xa000, 3, 4, 0);


        debug_log("cenet_setBreakpoint returned %d\n",i);
      }

      i=cenet_waitForDebugEvent(fd, pHandle, &devent, 2000);
      if (i)
      {
        CONTEXT c;
        debug_log("stopped with devent.debugevent %d\n", devent.debugevent);

        cenet_getThreadContext(fd, pHandle, devent.threadid, &c);


        if (devent.debugevent==5)
        {
          debug_log("TRAP (thread %d)\n", devent.threadid);



          debug_log("Going to remove breakpoint\n");
          cenet_removeBreakpoint(fd, pHandle, devent.threadid,0,1);
          debug_log("After removeBreakpoint\n");

          cenet_continueFromDebugEvent(fd, pHandle, devent.threadid, 2); //single step

          i=cenet_waitForDebugEvent(fd, pHandle, &devent, 2000);
          debug_log("after single step. i=%d\n",i);

          cenet_getThreadContext(fd, pHandle, devent.threadid, &c);


          debug_log("devent.debugevent=%d (thread %d)\n", devent.debugevent, devent.threadid);

        //  cenet_setBreakpoint(fd, pHandle, devent.threadid,0xce0000, 3,4,0);


         // cenet_setBreakpoint(fd, pHandle, -1, 0xa000, 3, 4);

         // debug_log("cenet_removeBreakpoint returned\n");
          cenet_continueFromDebugEvent(fd, pHandle, devent.threadid, 1); //continue unhandled/single step
        }
        else
          cenet_continueFromDebugEvent(fd, pHandle, devent.threadid, 0);
      }
    }

  }
  else
  {
    debug_log("Failed to start the debugger\n");
  }

  return NULL;

}

void *CESERVERTEST_RPMTHREAD(void *arg)
{
  int fd=cenet_connect();
  int i;
int hp;

  while (1)
  {
    //cenet_readProcessMemory(fd, 0x601048, &hp, 4)
#ifdef __arm__
    i=cenet_readProcessMemory(fd, pHandle, 0xa000, &hp, 4);
#else
    i=cenet_readProcessMemory(fd, pHandle, 0x601068, &hp, 4);
#endif
    debug_log("CESERVERTEST_RPMTHREAD:");

    debug_log("i=%d\n", i);
    debug_log("hp=%d\n", hp);
    sleep(1);
  }
}

void *CESERVERTEST(int pid )
{
  CONTEXT c;
  int fd;
  int arch;
  int dest;
  int i;

  pthread_t pth;
  debug_log("CESERVERTEST: running (v2)\n");

  //sleep(2);
  debug_log("connecting...\n");

  fd=cenet_connect();
  debug_log("fd=%d\n", fd);
  debug_log("pid=%d\n", pid);

  pHandle=cenet_OpenProcess(fd, pid);

  debug_log("pHandle=%d\n", pHandle);
  memset(&c, 0xce,sizeof(c));
/*
  debug_log("calling cenet_loadExtension\n");
  i=cenet_loadExtension(fd, pHandle);
  debug_log("cenet_loadExtension returned %d\n", i);
  return NULL;*/



  debug_log("Getting thread context\n");
  if (cenet_getThreadContext(fd, pHandle, pid, &c))
  {
    debug_log("Success:\n");

#ifdef __x86_64__
    debug_log("RIP=%x\n", c.regs.rip);
#endif

#ifdef __arm__
    debug_log("PC=%x\n", c.regs.ARM_pc);
    debug_log("ARM_cpsr=%x\n", c.regs.ARM_cpsr);
#endif
#ifdef __aarch64__

    if (c.type==2)
    {
      debug_log("Success: 32-bit result\n");
      debug_log("PC=%x\n", c.regs32.uregs[15]);
      debug_log("ARM_cpsr=%x\n", c.regs32.uregs[16]);
      for (i=0; i<32; i++)
      {
        debug_log("%.2d : ",i);

        float *f=(float *)&c.fp32.fpregs[i];
        uint32_t *d=(uint32_t *)&c.fp32.fpregs[i];

        debug_log("%.8x - %.8x == %.2f %.2f\n",d[0],d[1],f[0],f[1]);
      }
    }

    if (c.type==3)
    {

      __uint128_t v;

      debug_log("Success:\n");
      debug_log("PC=%llx\n", c.regs.pc);
      debug_log("fp.fpsr=%x\n", c.fp.fpsr);
      debug_log("fp.fpcr=%x\n", c.fp.fpcr);
      for (i=0; i<32; i++)
      {
        debug_log("%.2d : ",i);

        float *f=(float *)&c.fp.vregs[i];

        debug_log("%.2f %.2f %.2f %.2f\n",f[0],f[1],f[2],f[3]);
      }

      debug_log("---------------------------------");
      for (i=0; i<32; i++)
      {
        debug_log("%.2d : ",i);


        double *d=(double *)&c.fp.vregs[i];

        debug_log("%.2d %.2d\n",d[0],d[1]);
      }
    }
#endif


    return 1;
  }
  else
    debug_log("Fail\n");


  return 0;

 // debug_log("going to read memory\n");

 // while (1)
 //   i=cenet_readProcessMemory(fd, pHandle, 0xffff0000, &dest,4);

 // printf("i=%d",i);
//  arch=cenet_getArchitecture(fd, pHandle);

//  printf("arch=%d\n", arch);

 // fflush(stdout);
 // return NULL;


  //cenet_VirtualQueryExFull(fd, pHandle,  VQE_DIRTYONLY | VQE_PAGEDONLY);

 // CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);



  //launch the debuggerthread
 // pthread_create(&pth, NULL, CESERVERTEST_DEBUGGERTHREAD, NULL);

  debug_log("calling CESERVERTEST_DEBUGGERTHREAD\n");
  CESERVERTEST_DEBUGGERTHREAD(NULL);

  //launch the rpmthread
 // sleep(1);
  //pthread_create(&pth, NULL, CESERVERTEST_RPMTHREAD, NULL);

 // while (1);

  debug_log("End of test\n");
  fflush(stdout);

  return NULL;

}
