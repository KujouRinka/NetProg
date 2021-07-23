package server

import (
	"bufio"
	"io"
	"log"
	"net"
	"strconv"
	"strings"
)

func StartOpServer() {
	laddr, err := net.ResolveTCPAddr("tcp", ":8888")
	if err != nil {
		log.Fatalln(err)
	}
	listener, err := net.ListenTCP("tcp", laddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer listener.Close()

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println(err)
			continue
		}
		go handleConn(conn)
	}
}

func handleConn(conn net.Conn) {
	defer conn.Close()
	scanner := bufio.NewScanner(conn)
	scanner.Scan()
	numOp, _ := strconv.Atoi(scanner.Text())
	var data []int
	for i := 0; i < numOp; i++ {
		scanner.Scan()
		num, _ := strconv.Atoi(scanner.Text())
		data = append(data, num)
	}
	scanner.Scan()
	oper := scanner.Text()
	r := iterAndCalc(oper, data)
	io.Copy(conn, strings.NewReader(strconv.Itoa(r)))
}

func iterAndCalc(oper string, dataSet []int) int {
	base := dataSet[0]
	dataSet = dataSet[1:]
	for _, n := range dataSet {
		switch oper {
		case "+":
			base += n
		case "-":
			base -= n
		case "*":
			base *= n
		}
	}
	return base
}
