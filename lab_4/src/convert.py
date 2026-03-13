from PIL import Image
import os
import sys


def convert_folder(input_dir, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    extensions = ('.jpg', '.jpeg', '.png', '.bmp', '.gif')
    
    for filename in os.listdir(input_dir):
        if filename.lower().endswith(extensions):
            input_path = os.path.join(input_dir, filename)
            name = os.path.splitext(filename)[0]
            output_path = os.path.join(output_dir, name + '.ppm')
            
            try:
                img
= Image.open(input_path) img =
    img.convert('RGB') img.save(output_path, 'ppm') print(f "{filename} -> {name}.ppm")
        except Exception as e : print(f "{filename} -> Error: {e}")

                                    if __name__ == "__main__" : if len (sys.argv) == 3 : input_dir =
        sys.argv[1] output_dir =
            sys.argv[2] else : raise Exception("Use convert.py <input_dir> <output_dir>")

                                   if not os.path.isdir(input_dir)
    : raise Exception(f "Error: Directory '{input_dir}' not found!")

          convert_folder(input_dir, output_dir)