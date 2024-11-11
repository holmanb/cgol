package main

import (
	"encoding/csv"
	"flag"
	"fmt"
	"math/rand"
	"os"
	"runtime"
	"strings"
	"sync"
	"time"
)

var config = &struct {
	file, directory    string
	benchmark, noPrint bool
	goroutines, iter   int
	sleep, matrixSize  uint
}{
	file:       "default",
	directory:  "../../data/",
	benchmark:  false,
	noPrint:    false,
	iter:       10,
	sleep:      100,
	matrixSize: 0,
	goroutines: 0,
}

type format struct {
	live, dead string
	delim      rune
}

const (
	defaultFile = "../../data/default"
	bitSize     = 32
)

type Array [][]uint32

/* bitwise ops */
func (a *Array) SetBit(x, y int) {
	(*a)[x][y/bitSize] |= (1 << (y % bitSize))
}

func (a *Array) UnSetBit(x, y int) {
	(*a)[x][y/bitSize] &= ^(1 << (y % bitSize))
}

func (a *Array) GetBit(x, y int) bool {
	return (*a)[x][y/bitSize]&(1<<(y%bitSize)) != 0
}

// set entire uint32 (for clear & random number generation)
func (a *Array) SetVal(x, y int, val uint32) {
	(*a)[x][y] = val
}

func (a *Array) Clear(x, y int) {
	var waitgroup sync.WaitGroup
	for i := 0; i < x; i++ {
		waitgroup.Add(1)
		go func(i, x, y int, a *Array, wg *sync.WaitGroup) {
			defer wg.Done()
			for cell := range (*a)[i] {
				a.SetVal(i, cell, 0)
			}
		}(i, x, y, a, &waitgroup)
	}
	waitgroup.Wait()
}

type Vertex struct {
	x int
	y int
}

type Matrix struct {
	primary   Array
	secondary Array
	x         int
	y         int
}

func clearScreen() {
	fmt.Println("\033[H\033[J")
}

func (m *Matrix) Generate(x, y uint) {
	m.x = int(x)
	m.y = int(y)
	// integer division, adjust array size for sizes not divisible by 32
	arraySize := m.y / bitSize
	if m.y%bitSize != 0 {
		arraySize += 1
	}
	rand.Seed(time.Now().UnixNano())
	m.primary = make(Array, m.x)
	m.secondary = make(Array, m.x)
	for i := 0; i < m.x; i++ {

		m.primary[i] = make([]uint32, arraySize)
		m.secondary[i] = make([]uint32, arraySize)
		for cell := range m.primary[i] {
			m.primary.SetVal(i, cell, rand.Uint32())
		}
	}
}

func (m *Matrix) ReadFile(file string, f format) {
	csvfile, err := os.Open(file)
	if err != nil {
		panic("Couldn't open the csv file")
	}
	r := csv.NewReader(csvfile)
	r.Comma = f.delim
	csvStr, err := r.ReadAll()
	if err != nil {
		panic(err)
	}
	m.x = len(csvStr)
	m.primary = make(Array, m.x)
	m.secondary = make(Array, m.x)
	for i := range csvStr {
		if i == 0 {
			m.y = len(csvStr[i])
		} else if len(csvStr[i]) != m.y {
			panic("error parsing file, jagged array")
		}
		m.primary[i] = make([]uint32, len(csvStr[i])/bitSize+1)
		m.secondary[i] = make([]uint32, len(csvStr[i])/bitSize+1)
		for j := range csvStr {
			if csvStr[i][j] == "1" {
				m.primary.SetBit(i, j)
			}
		}
	}
}

func (m Matrix) String() string {
	var sb strings.Builder
	for i := range m.primary {
		for j := range m.primary {
			if m.primary.GetBit(i, j) {
				sb.WriteString(" 1 ")
			} else {
				sb.WriteString("   ")
			}
		}
		sb.WriteString("\n")
	}
	return sb.String()
}

func run(v Vertex, matrix *Matrix, wg *sync.WaitGroup) {
	defer wg.Done()
	var X, Y, count, m, n int
	count = 0
	// count live cells in matrix
	for m = -1; m < 2; m++ {
		for n = -1; n < 2; n++ {
			if m == 0 && n == 0 {
				// don't include current cell in the count
				continue
			}
			// check x axis wrapping
			if v.x == 0 && m == -1 {
				// wrap left
				X = matrix.x - 1
			} else if v.x == matrix.x-1 && m == 1 {
				// wrap right
				X = 0
			} else {
				// no wrap
				X = v.x + m
			}

			// check y axis wrapping
			if v.y == 0 && n == -1 {
				// wrap down
				Y = matrix.y - 1
			} else if v.y == matrix.y-1 && n == 1 {
				// wrap up
				Y = 0
			} else {
				// no wrap
				Y = v.y + n
			}
			if matrix.primary.GetBit(X, Y) {
				count++
			}
		}
	}
	if count == 3 {
		matrix.secondary.SetBit(v.x, v.y)
	} else if count == 2 && matrix.primary.GetBit(v.x, v.y) {
		matrix.secondary.SetBit(v.x, v.y)
	}
}
func (matrix *Matrix) Cgol() {
	var waitgroup sync.WaitGroup
	for i := 0; i < matrix.x; i++ {
		for j := 0; j < matrix.x; j++ {
			waitgroup.Add(1)
			go run(Vertex{i, j}, matrix, &waitgroup)
		}
	}
	waitgroup.Wait()
	matrix.primary, matrix.secondary = matrix.secondary, matrix.primary
	matrix.secondary.Clear(matrix.x, matrix.y)
}

func main() {
	flag.StringVar(&config.file, "f", config.file, "input file to use")
	flag.StringVar(&config.directory, "d", config.directory, "input directory to use")
	flag.BoolVar(&config.benchmark, "b", config.benchmark, "benchmark defaults")
	flag.BoolVar(&config.noPrint, "n", config.noPrint, "no print - only print the final result")
	flag.IntVar(&config.iter, "i", config.iter, "iterations to run")
	flag.IntVar(&config.goroutines, "t", config.goroutines, "number of goroutines to allow")
	flag.UintVar(&config.sleep, "s", config.sleep, "sleep time betweeen iterations")
	flag.UintVar(&config.matrixSize, "m", config.matrixSize, "size of matrix to generate")
	flag.Parse()
	if config.goroutines > 0 {
		runtime.GOMAXPROCS(config.goroutines)
	}

	f := format{delim: ';', live: "1", dead: "0"}
	matrix := Matrix{}
	if config.matrixSize != 0 {
		matrix.Generate(config.matrixSize, config.matrixSize)
	} else {
		matrix.ReadFile(config.directory+config.file, f)
	}
	start := time.Now()
	for i := 0; i != config.iter; i++ {
		if !config.noPrint && !config.benchmark {
			clearScreen()
			fmt.Println(matrix)
		}
		matrix.Cgol()
		if !config.benchmark {
			time.Sleep(time.Millisecond * time.Duration(config.sleep))
		}
	}
	t := time.Now()
	elapsed := t.Sub(start)
	fmt.Printf("runtime: %v\n", elapsed)
	fmt.Printf(
		"memory used: %v * %v = %v bytes of data for %v x %v array\n",
		cap(matrix.primary),
		cap(matrix.primary[0]),
		cap(matrix.primary)*cap(matrix.primary[0]),
		matrix.x,
		matrix.y,
	)
	if config.benchmark {
		fmt.Printf("MBps %v\n", float64(matrix.x)*float64(matrix.y)*float64(config.iter)/float64(1024)/float64(1024)/elapsed.Seconds())
	} else if config.noPrint {
		fmt.Println(matrix)
	}
}
