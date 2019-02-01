package main

/*
#include <stdlib.h>
#include "TinyModbus.h"

extern void TM_WriteImpl(TinyModbus *tf, uint8_t *buff, uint32_t len);
extern bool TM_ClaimTx(TinyModbus *tf);
extern void TM_ReleaseTx(TinyModbus *tf);
extern void DataListener(TinyModbus *tf, TM_ResponseMsg *msg);

inline static void AppendListeners(TinyModbus *tm){
	TM_AddGenericListener(tm, DataListener);
};
*/
import "C"

import (
	"fmt"
	"io"
	"reflect"
	"time"
	"unsafe"

	"github.com/jacobsa/go-serial/serial"
)

var (
	serialIO *io.ReadWriteCloser
	tm       *C.TinyModbus
)

//export TM_ReleaseTx
func TM_ReleaseTx(tm *C.TinyModbus) {
	fmt.Println("RELEASE")
}

//export TM_ClaimTx
func TM_ClaimTx(tm *C.TinyModbus) C.bool {
	fmt.Println("CLAIM")
	return true
}

func rawToByteArray(buff *C.uint8_t, len C.uint32_t) []byte {
	var bb []byte
	sliceHeader := (*reflect.SliceHeader)((unsafe.Pointer(&bb)))
	sliceHeader.Cap = int(len)
	sliceHeader.Len = int(len)
	sliceHeader.Data = uintptr(unsafe.Pointer(buff))
	return bb
}

//export TM_WriteImpl
func TM_WriteImpl(tf *C.TinyModbus, buff *C.uint8_t, len C.uint32_t) {

	rawBytes := rawToByteArray(buff, C.uint32_t(len))
	fmt.Printf("REQUEST => %#v\n", rawBytes)

	bb := rawToByteArray(buff, len)
	(*serialIO).Write(bb)
}

//export DataListener
func DataListener(tf *C.TinyModbus, msg *C.TM_ResponseMsg) {

	rawBytes := rawToByteArray(msg.data, C.uint32_t(msg.length))
	fmt.Print("=>\x1b[34mPARSED FRAME:\n")
	fmt.Printf("  \x1b[35m peer_address:\x1b[0m %#v\n", msg.peer_address)
	fmt.Printf("  \x1b[35m function:\x1b[0m %#v\n", msg.function)
	fmt.Printf("  \x1b[31m is_error:\x1b[0m %t\n", msg.is_error)
	fmt.Printf("  \x1b[31m error_code:\x1b[0m %#v\n", msg.error_code)
	fmt.Printf("  \x1b[36m data:\x1b[0m %#v\n", rawBytes)
	fmt.Printf("  \x1b[36m data_length:\x1b[0m %#v\n", msg.length)

}

func serialBridge() {

	tm = C.TM_Init(C.TM_SLAVE)
	C.AppendListeners(tm)

	options := serial.OpenOptions{
		PortName:              "COM30",
		BaudRate:              19200,
		DataBits:              8,
		StopBits:              1,
		MinimumReadSize:       1,
		InterCharacterTimeout: 10,
	}

	f, err := serial.Open(options)
	serialIO = &f

	if err != nil {
		fmt.Println("Error opening serial port: ", err)
		return
	} else {
		defer f.Close()
	}

	go func() {
		for {
			C.TM_Tick(tm)
			time.Sleep(time.Millisecond)
		}
	}()

	for {
		buf := make([]byte, 32)
		n, err := f.Read(buf)
		if err != nil {
			if err != io.EOF {
				fmt.Println("Error reading from serial port: ", err)
			}
		} else {
			buf = buf[:n]

			if len(buf) > 0 {
				fmt.Printf("RESPONSE => %#v\n", buf)
			}
			for _, v := range buf {
				C.TM_AcceptChar(tm, C.uchar(v))
			}

		}
	}

}

func main() {
	go serialBridge()

	for {
		time.Sleep(time.Second)
		fmt.Println("\x1b[33m=====CMD START===== \x1b[0m")
		C.TM_SendSimple(tm, 0x01, 0x06, 0x2000, 0x0008) // write start
		time.Sleep(time.Second)
		fmt.Println("\x1b[33m=====CMD STOP=====\x1b[0m")
		C.TM_SendSimple(tm, 0x01, 0x06, 0x2000, 0x0004) // write stop
		time.Sleep(time.Second)
		fmt.Println("\x1b[33m=====CMD SET FREQ=====\x1b[0m")
		C.TM_SendSimple(tm, 0x01, 0x06, 0x010D, 0x2000) // write frequency
		time.Sleep(time.Second)
		fmt.Println("\x1b[33m=====CMD GET STATUS=====\x1b[0m")
		C.TM_SendSimple(tm, 0x01, 0x03, 0x1005, 0x0001) // get status
		time.Sleep(time.Second)
		fmt.Println("\x1b[33m=====CMD RESET ERROR=====\x1b[0m")
		C.TM_SendSimple(tm, 0x01, 0x06, 0x2000, 0x0009) // reset error
		time.Sleep(time.Second)
		fmt.Println("\x1b[33m=====CMD GET MULTIPLE REGISTERS=====\x1b[0m")
		C.TM_SendSimple(tm, 0x01, 0x03, 0x1005, 0x0010) // get status
	}

}
