from pusher import Pusher

pusher = Pusher(
    app_id='APP_ID',
    key = "KEY",
    secret = "SECRET",
    cluster = "CLUSTER",
    ssl=True
)

## GCP Config
project_id = "PROJECT_ID"
topic_name = "projects/PROJECT_ID/topics/TOPIC"
subscription_name = "SUBSCRIPTION_NAME"