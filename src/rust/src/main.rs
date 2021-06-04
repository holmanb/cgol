extern crate csv;
extern crate serde_derive;

use serde::Deserialize;
use std::fs::File;
use std::error::Error;
use csv::ReaderBuilder;

#[derive(Debug, Deserialize, Eq, PartialEq)]
struct Row {
    values: Vec<u8>,
}

fn main() {
    let mut a = Arr2D::default();
    if let Ok(_) = a.new_load("../../../data/flier".to_string()) {
        a.print();
    }

    let mut b = Arr2D::new(3);
    b.print();
    println!("changing value: {} to 1\n", b.set(1, 1, 1));
    b.print();
}

#[derive(Debug, Default)]
struct Arr2D {
    arr: std::vec::Vec<u8>,
    width: usize,
}

impl Arr2D {

    fn new(size: usize) -> Arr2D {
        Arr2D{
            width: size,
            arr: vec![0; size*size],
        }
    }

    fn new_load(&mut self, file: String) -> Result<(), Box<dyn Error>> {
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
                self.set(x, y, item);
                x+=1;
            }
            y+=1;
        }
        Ok(())
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
