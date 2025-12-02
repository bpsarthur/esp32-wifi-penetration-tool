import gzip
import os

def main():
    try:
        with open('index.html', 'r', encoding='utf-8') as f:
            html = f.read()
        
        with open('style.css', 'r', encoding='utf-8') as f:
            css = f.read()
        
        with open('main.js', 'r', encoding='utf-8') as f:
            js = f.read()
        
        # Inline CSS and JS
        # Note: Strings must match index.html exactly
        css_tag = '<link rel="stylesheet" href="style.css" />'
        js_tag = '<script src="main.js"></script>'
        
        if css_tag not in html:
            print(f"Warning: CSS tag '{css_tag}' not found in HTML")
        else:
            html = html.replace(css_tag, f'<style>{css}</style>')
            
        if js_tag not in html:
            print(f"Warning: JS tag '{js_tag}' not found in HTML")
        else:
            html = html.replace(js_tag, f'<script>{js}</script>')
        
        # Gzip
        compressed = gzip.compress(html.encode('utf-8'))
        
        # Generate C array
        hex_array = ', '.join([f'0x{b:02X}' for b in compressed])
        
        c_code = f"""#ifndef PAGE_INDEX_H
#define PAGE_INDEX_H

unsigned char page_index[] = {{
    {hex_array}
}};
unsigned int page_index_len = {len(compressed)};

#endif
"""
        
        output_path = '../components/webserver/pages/page_index.h'
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(c_code)
        
        print(f"page_index.h generated successfully at {output_path}")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == '__main__':
    main()
