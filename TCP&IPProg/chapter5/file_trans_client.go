package server

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"path"
)

func StartFileTransClient(remoteAddr string) {
	raddr, err := net.ResolveTCPAddr("tcp", remoteAddr)
	if err != nil {
		log.Fatalln(err)
	}
	conn, err := net.DialTCP("tcp", nil, raddr)
	if err != nil {
		log.Fatalln(err)
	}

	scanner := bufio.NewScanner(os.Stdin)
	fmt.Print("filename: ")
	scanner.Scan()
	filename := scanner.Text()
	reqPack := []byte{byte(len(filename))}
	reqPack = append(reqPack, []byte(filename)...)
	conn.Write(reqPack)

	ok := make([]byte, 1)
	conn.Read(ok)
	if int(ok[0]) == 0 {
		return
	}
	filename = path.Base(filename)
	f, err := os.OpenFile(filename, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0666)
	if err != nil {
		log.Fatalln(err)
	}
	io.Copy(f, conn)
}
