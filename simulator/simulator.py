import os

os.system("cd ../GUI && export FLASK_ENV=development")
os.system("cd ../GUI && export FLASK_APP=app.py")
os.system("cd ../GUI && flask run")