extern crate csv;
extern crate serde_derive;

use std::mem::swap;
use std::fs::File;
use std::error::Error;
use std::{thread,time};
use serde::Deserialize;
use csv::ReaderBuilder;

#[derive(Debug, Deserialize, Eq, PartialEq)]
struct Row {
    values: Vec<u8>,
}

fn main() {
    let mut a = Arr2D::default();
    if let Ok(_) = a.load("../../../data/flier".to_string()) {
        a.print();
    }

    let mut b = Arr2D::new(3);
    b.print();
    println!("changing value: {} to 1\n", b.set(1, 1, 1));
    b.print();

    let mut l = Life::default();
    if let Ok(_) = l.load("../../../data/flier".to_string()) {
        l.print();
        l.life();
        l.print();
    }
    life_loop();
}

#[derive(Debug, Default)]
struct Arr2D {
    arr: std::vec::Vec<u8>,
    width: usize,
}

#[derive(Debug, Default)]
struct Life {
    arr1: Arr2D,
    arr2: Arr2D,
    size: usize,
}

impl Life {

    #[allow(dead_code)]
    fn new(size: usize) -> Life {
        Life{
            arr1: Arr2D::new(size),
            arr2: Arr2D::new(size),
            size: size
        }
    }

    fn load(&mut self, file: String) -> Result<usize, Box<dyn Error>> {
        let var = self.arr1.load(file)?;
        self.arr2 = Arr2D::new(var);
        self.size = var;
        Ok(var)
    }
    fn print(&self){
        self.arr1.print();
    }

    fn life(&mut self) {
        for i in 0..self.arr1.width {
            for j in 0..self.arr1.width{

                #[allow(non_snake_case)]
                let mut count = 0;
                #[allow(non_snake_case)]
                let (mut X, mut Y);

                for m in -1..=1 {
                    for n in -1..=1 {
                        if m == 0 && n == 0 {
                            continue;
                        }
                        if i == 0 && m == -1 {
                            X = self.size - 1;
                        } else if i == self.size - 1 && m == 1 {
                            X = 0;
                        } else {
                            X = add(i, m);
                        }

                        if j == 0 && n == -1 {
                            Y = self.size - 1;
                        } else if j == self.size - 1 && n == 1 {
                            Y = 0;
                        } else {
                            Y = add(j, n);
                        }
                        if self.arr1.get(X, Y) == 1 {
                            count+=1;
                        }
                    }
                }
                if count == 3 {
                    self.arr2.set(i, j, 1);
                } else if self.arr1.get(i, j) == 1 && count == 2 {
                    self.arr2.set(i, j, 1);
                }
            }
        }
        swap(&mut self.arr1, &mut self.arr2);
        self.arr2.clear()
    }
}

impl Arr2D {

    #[allow(dead_code)]
    fn new(size: usize) -> Arr2D {
        Arr2D{
            width: size,
            arr: vec![0; size*size],
        }
    }

    fn load(&mut self, file: String) -> Result<usize, Box<dyn Error>> {
        let file = File::open(file).expect("Unable to open");
        let mut rdr = ReaderBuilder::new()
            .has_headers(false)
            .delimiter(b';')
            .from_reader(file);
        let mut i = rdr.deserialize();

        let mut x;
        let mut y = 0;
        while let Some(result) = i.next() {
            let record: Row = result?;
            if self.width == 0 {
                self.width = record.values.len();
                self.arr = vec![0; self.width*self.width];

            }
            self.width = record.values.len();
            x=0;
            for item in record.values.clone(){
                self.set(y, x, item);
                x+=1;
            }
            y+=1;
        }
        Ok(self.width)
    }

    fn clear(&mut self){
        self.setall(0);
    }

    fn setall(&mut self, val: u8){
        for i in 0..self.width {
            for j in 0..self.width{
                self.set(i, j, val);
            }
        }
    }

    fn get(&self, x: usize, y: usize) -> u8 {
        self.arr[self.width*x + y]
    }

    // return old value before setting
    fn set(&mut self, x: usize, y: usize, val: u8) -> u8 {
        let old = self.get(x, y);
        self.arr[self.width * x + y] = val;
        old
    }

    fn print(&self) {
        for i in 0..self.width {
            for j in 0..self.width{
                print!("{} ", self.arr[self.width*i + j]);
            }
            println!();
        }
    }
}

#[allow(dead_code)]
fn add(u: usize, i: i32) -> usize {
    if i.is_negative() {
        u - i.wrapping_abs() as u32 as usize
    } else {
        u + i as usize
    }
}

#[allow(dead_code)]
fn clear(){
    print!("{esc}[2J{esc}[1;1H", esc = 27 as char);
}

#[allow(dead_code)]
fn life_loop(){
    let sleep = time::Duration::from_millis(100);
    let mut l = Life::default();
    if let Ok(_) = l.load("../../../data/flier".to_string()) {
        loop {
            clear();
            l.print();
            l.life();
            thread::sleep(sleep);
        }
    }
}
