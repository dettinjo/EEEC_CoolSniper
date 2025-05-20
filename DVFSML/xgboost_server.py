import joblib
import numpy as np
from fastapi import FastAPI
from pydantic import BaseModel
from sklearn.preprocessing import StandardScaler
import xgboost as xgb

# Load the trained model
best_xgb_model = joblib.load('best_throttle_predictor.joblib')

# Load the scaler
scaler = joblib.load('scaler.joblib')

# Initialize FastAPI app
app = FastAPI()

# Define input data model
class InputData(BaseModel):
    data: str

# Define prediction endpoint
@app.post("/predict")
async def predict_throttle(data: InputData):
    # Split the input string into 16 values
    values = data.data.split(',')
    if len(values) != 16:
        raise ValueError("Input should contain 16 comma-separated values")

    # Convert values to float and reshape into 4 features
    features = np.array([float(v) for v in values]).reshape(4, 4)
    
    # Scale features
    features_scaled = scaler.transform(features)
    
    # Get prediction
    prediction = best_xgb_model.predict(features_scaled)
    probability = best_xgb_model.predict_proba(features_scaled)[:, 1]
    
    return {
        "is_throttled": [bool(p) for p in prediction],
        "probability": [float(p) for p in probability]
    }

# Run the server
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
