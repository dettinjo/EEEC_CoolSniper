import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report, confusion_matrix
import joblib
import numpy as np
import xgboost as xgb

# Load and preprocess data
data = pd.read_csv('data.csv')
print(f"Dataset shape: {data.shape}")
print("\nDataset info:")
data.info()

print("\nSample data:")
print(data.head())

print("\nFeature statistics:")
print(data.describe())

print("\nClass distribution:")
print(data['InThrottleMode'].value_counts(normalize=True))

# Check for missing values
missing_values = data.isnull().sum()
if missing_values.sum() > 0:
    print("\nMissing values:")
    print(missing_values[missing_values > 0])
else:
    print("\nNo missing values found in the dataset.")

# Correlation analysis
correlation_matrix = data.corr()
plt.figure(figsize=(10, 8))
sns.heatmap(correlation_matrix, annot=True, cmap='coolwarm')
plt.title('Correlation Matrix of Features')
plt.savefig('correlation_matrix.png')
plt.close()

X = data[['Power', 'Frequency', 'Temperature', 'Utilization']].values
y = data['InThrottleMode'].values

# Scale features
scaler = StandardScaler()
X = scaler.fit_transform(X)

# Save the scaler
joblib.dump(scaler, 'scaler.joblib')

# Split data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
print(f"\nTraining set shape: {X_train.shape}")
print(f"Test set shape: {X_test.shape}")

# Define parameter grid for GridSearchCV
param_grid = {
    'max_depth': [3, 4, 5, 6, 7],
    'learning_rate': [0.01, 0.1, 0.3],
    'n_estimators': [100, 200, 300],
    'subsample': [0.8, 0.9, 1.0],
    'colsample_bytree': [0.8, 0.9, 1.0]
}

# Initialize GridSearchCV with XGBoost
xgb_model = xgb.XGBClassifier(random_state=42, use_label_encoder=False, eval_metric='logloss')
grid_search = GridSearchCV(xgb_model, param_grid, cv=5, n_jobs=-1, verbose=2)

# Perform grid search
print("\nPerforming grid search...")
grid_search.fit(X_train, y_train)

# Print best parameters and score
print("\nBest parameters found:")
for param, value in grid_search.best_params_.items():
    print(f"{param}: {value}")
print(f"\nBest cross-validation score: {grid_search.best_score_:.4f}")

# Get the best model
best_xgb_model = grid_search.best_estimator_

# Evaluate the best model
y_pred = best_xgb_model.predict(X_test)
accuracy = best_xgb_model.score(X_test, y_test)
print(f'\nTest Accuracy: {accuracy:.4f}')

print("\nClassification Report:")
print(classification_report(y_test, y_pred))

print("\nConfusion Matrix:")
cm = confusion_matrix(y_test, y_pred)
print(cm)

# Plot confusion matrix
plt.figure(figsize=(10, 8))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues')
plt.title('Confusion Matrix')
plt.ylabel('True label')
plt.xlabel('Predicted label')
plt.xticks([0.5, 1.5], ['Not In ThrottleMode', 'In ThrottleMode'])
plt.yticks([0.5, 1.5], ['Not In ThrottleMode', 'In ThrottleMode'])
plt.savefig('confusion_matrix.png')
plt.close()

# Save the best XGBoost model
joblib.dump(best_xgb_model, 'best_throttle_predictor.joblib')
