from flask import Flask
from flask import request
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from datetime import datetime

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = "postgresql://postgres:passwordPSQL147@rds-psql-cs147.cocc3xkmyxa4.us-east-1.rds.amazonaws.com:5432/fitbuddydb"
db = SQLAlchemy(app)
migrate = Migrate(app, db);

class InfoModel(db.Model):
    __tablename__ = "info"

    id = db.Column(db.Integer, primary_key=True)
    date = db.Column(db.Date())
    duration = db.Column(db.Integer())
    step_count = db.Column(db.Integer())

    def __init__(self, date, duration, step_count):
        self.date = date
        self.duration = duration
        self.step_count = step_count

@app.route("/")
def hello():
    print(request.args.get("var"))
    return "Hello World!"

@app.route("/add", methods=["GET"])
def add_info():
    date = datetime.today().strftime('%Y-%m-%d')
    duration = request.args['dur']
    step_count = request.args['cnt']
    new_data = InfoModel(date=date, duration=duration, step_count=step_count)
    db.session.add(new_data)
    db.session.commit()
    return {"message": "exercise added successfully"}

@app.route("/get", methods=["GET"])
def get_info():
    all_data = InfoModel.query.all()
    json_data = "{"
    csv = ""
    for data in all_data:
        csv += str(data.id) + ",\"" + str(data.date.strftime("%Y-%m-%d")) + "\"," + str(data.duration) + "," + str(data.step_count) + "\n"
    return csv

if __name__ == '__main__':
    app.run(debug=True);