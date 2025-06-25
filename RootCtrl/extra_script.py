# extra_script.py
import os

def convert_html_to_header(html_path, header_path, var_name):
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    with open(header_path, "w", encoding="utf-8") as f:
        f.write(f'const char* {var_name} = R"rawliteral(\n{html}\n)rawliteral";\n')

def before_build(project, config, environment):
    html_dir = os.path.join("data_inject")  # nom de ton dossier source HTML
    output_dir = os.path.join("src", "html_gen")  # destination dans src/

    os.makedirs(output_dir, exist_ok=True)

    # Map des fichiers HTML à transformer
    mappings = {
        "dashboard.html": ("dashboard_html.h", "DASHBOARD_HTML"),
        "settings.html": ("settings_html.h", "SETTINGS_HTML"),
        "logs.html": ("logs_html.h", "LOGS_HTML"),
    }

    for html_file, (header_file, var_name) in mappings.items():
        html_path = os.path.join(html_dir, html_file)
        header_path = os.path.join(output_dir, header_file)
        convert_html_to_header(html_path, header_path, var_name)

# Hook sur le process de build
Import("env")
before_build(None, None, None)
