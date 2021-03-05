package main

import (
	"fmt"
	"os"
	"io"
	"encoding/csv"
)

type format struct {
	live, dead string
	delim rune
}

const (
	defaultFile = "../../data/default"
)


func main() {
	f := format{delim: ';', live:"1", dead:"0"}
	var i int
	//m2 := make([][]bool)
	csvfile, err := os.Open(defaultFile)
	if err != nil {
		panic("Couldn't open the csv file")
	}
	r := csv.NewReader(csvfile)
	r.Comma = f.delim

	for {
		record, err := r.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			panic(err)
		}
		m1 := make([][]bool, len(record))
		for i := range m1 {
			    m1[i] = make([]bool, len(record))
		}
		for j,_ := range record {
			if record[j] == f.live {
				fmt.Print(record[j])
				fmt.Print(" ")

				m1[i][j] = true
			}
			if record[j] == f.dead {
				fmt.Print(record[j])
				fmt.Print(" ")
				m1[i][j] = false
			}
		}
		fmt.Println()
		i++
	}
}
