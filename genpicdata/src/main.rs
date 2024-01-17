use image::imageops::FilterType;
use std::{
    fs,
    io::{self, Write},
};

const INPUT_FILE: &str = "original.jpg";
const WIDTH: u32 = 640;
const HEIGHT: u32 = 480;

fn main() {
    let img = image::open(INPUT_FILE).expect("load INPUT_FILE");
    let img = img
        .resize_exact(WIDTH, HEIGHT, FilterType::Gaussian)
        .to_rgba8();

    let mut out =
        io::BufWriter::new(fs::File::create("picturedata.c").expect("create picturedata.c"));
    // write header of C file
    out.write(b"#include \"types.h\"\nuint8_t picturedata[] = {")
        .unwrap();

    // we need to write it in BGRA8, and there's no easy way to conver it
    // so here's my hacky way
    const ORDER: [usize; 4] = [2, 1, 0, 3];
    let mut i = 0;
    for p in img.pixels() {
        for &j in &ORDER {
            if i % 20 == 0 {
                out.write(b"\n").unwrap();
            }
            // write each byte in array from header
            let fmt = format!(" 0x{:02X},", p.0[j]);
            out.write(fmt.as_bytes()).unwrap();
            i += 1;
        }
    }
    // finish the array
    out.write(b"\n};\n").unwrap();

    img.save("render.png").unwrap();

    println!("{} : true bytes", i);
    println!("{} : expected bytes", 640 * 480 * 4);
}
