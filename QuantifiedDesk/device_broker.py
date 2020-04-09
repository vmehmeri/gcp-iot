from google.cloud import pubsub_v1
from config import *
import os
import datetime

# Global state vars
# TO DO: Consider storing in DB
previous_time_sitting = previous_time_standing = 0
current_state = None
latest_msg_datetime = None

def add_activity(action):
    print("Triggering SIT/STAND event")
    ## get current time minus 1 minute
    _event_time = datetime.datetime.now() - datetime.timedelta(minutes=1)
    year, month, day, hour, minute = _event_time.year, _event_time.month, _event_time.day, _event_time.hour, _event_time.minute
    event_action = "Sat Down" if action == "SIT" else "Stood Up"
    event_date = "%s-%02d-%02d" %(year, month, day)
    event_time = "%02d:%02d" %(hour, minute)
    pusher.trigger(u'activity', u'add', {
        u'date': event_date,
        u'event': event_action,
        u'time':event_time
    })

def update_graph(time_standing, time_sitting):
    print("Updating bar chart")
    pusher.trigger(u'graph', u'update', {
                    u'units': "%s,%s,0" % (time_standing, time_sitting)
    })

def update_dashboard(time_sitting, time_standing):
    global previous_time_sitting, previous_time_standing, current_state
    sit_time_int = int(time_sitting)
    stand_time_int = int(time_standing)
    if not current_state and sit_time_int > stand_time_int:
        current_state = "SIT"
    elif not current_state and stand_time_int > sit_time_int:
        current_state = "STAND"
    else:
        if current_state == "STAND" and time_sitting > previous_time_sitting:
            print("State changed from standing to sitting")
            current_state = "SIT"
            add_activity("SIT")
        elif current_state == "SIT" and time_standing > previous_time_standing:
            add_activity("STAND")
            current_state = "STAND"
            print("State changed from sitting to standing")
    
    previous_time_sitting = time_sitting
    previous_time_standing = time_standing

    # Update graph
    update_graph(time_standing, time_sitting)

def callback(message):
    global latest_msg_datetime
    _msg_data = message.data
    message.ack()
    print ("--> Incoming message")
    msg_data = _msg_data.decode("utf-8") 
    #print(msg_data)
    data_arr = msg_data.split(',')
    if len(data_arr) != 4:
        print("Data is not recognized as telemetry")
        return
    
    
    time_sitting = data_arr[0]
    time_standing = data_arr[1]
    msg_date = data_arr[2]
    msg_time = data_arr[3]

    year,month,day = int(msg_date.split('/')[2]),int(msg_date.split('/')[1]),int(msg_date.split('/')[0])
    hour,minute,sec = int(msg_time.split(':')[0]),int(msg_time.split(':')[1]),int(msg_time.split(':')[2])
    msg_datetime = datetime.datetime(2000 + year,month,day,hour,minute,sec)
    if not latest_msg_datetime or msg_datetime > latest_msg_datetime:
        update_dashboard(time_sitting,time_standing)
        latest_msg_datetime = msg_datetime
    else:
        # msg is old, do nothing
        print("Older timestamp, discarding message...")

if __name__ == "__main__":
    subscriber = pubsub_v1.SubscriberClient()

    subscription_name = 'projects/{project_id}/subscriptions/{sub}'.format(
        project_id=project_id,
        sub=subscription_name, 
    )
    
    future = subscriber.subscribe(subscription_name, callback)
    try:
        future.result()
    except KeyboardInterrupt:
        future.cancel()