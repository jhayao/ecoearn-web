# IoT API Documentation

This document describes the API endpoints for IoT device communication with the EcoEarn platform.

## Base URL

```
Production: https://your-domain.com/api/iot
Development: http://localhost:3000/api/iot
```

## Authentication

All IoT requests require an API key that is generated when a new bin is created. The API key must be included in the request body.

### API Key Format
```
BIN_TIMESTAMP_RANDOMSTRING
Example: BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q
```

## Endpoints

### 1. Update Bin Location

Updates the GPS coordinates of a bin.

**Endpoint**: `POST /api/iot/update-location`

**Headers**:
```http
Content-Type: application/json
```

**Request Body**:
```json
{
  "apiKey": "BIN_XXXXXXXXX_XXXXXX",
  "latitude": 8.476876,
  "longitude": 123.799913
}
```

**Parameters**:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| apiKey | string | Yes | The unique API key for the bin |
| latitude | number | Yes | GPS latitude (-90 to 90) |
| longitude | number | Yes | GPS longitude (-180 to 180) |

### 2. Update Bin Capacity

Updates the fill level of bin compartments using ultrasonic sensors.

**Endpoint**: `POST /api/iot/update-capacity`

**Headers**:
```http
Content-Type: application/json
```

**Request Body**:
```json
{
  "apiKey": "BIN_XXXXXXXXX_XXXXXX",
  "comp1Capacity": 45.2,
  "comp2Capacity": 67.8
}
```

**Parameters**:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| apiKey | string | Yes | The unique API key for the bin |
| comp1Capacity | number | Yes | Compartment 1 fill percentage (0-100) |
| comp2Capacity | number | Yes | Compartment 2 fill percentage (0-100) |

**Success Response** (200 OK):
```json
{
  "success": true,
  "message": "Bin location updated successfully",
  "timestamp": "2025-11-06T12:34:56.789Z"
}
```

**Error Responses**:

**400 Bad Request** - Missing or invalid parameters:
```json
{
  "success": false,
  "error": "Missing required fields: apiKey, latitude, longitude"
}
```

**401 Unauthorized** - Invalid API key:
```json
{
  "success": false,
  "error": "Invalid API key or failed to update location"
}
```

**500 Internal Server Error** - Server error:
```json
{
  "success": false,
  "error": "Internal server error"
}
```

### 2. Health Check

Check if the API endpoint is working.

**Endpoint**: `GET /api/iot/update-location`

**Response** (200 OK):
```json
{
  "message": "IoT Update Location Endpoint",
  "method": "POST",
  "requiredFields": ["apiKey", "latitude", "longitude"]
}
```

## Rate Limiting

- **Recommended**: Maximum 1 request per minute per bin
- **Minimum interval**: 30 seconds between requests
- Excessive requests may result in temporary blocking

## Error Handling

### Common Error Codes

| Code | Description | Action |
|------|-------------|--------|
| 400 | Bad Request | Check request format and parameters |
| 401 | Unauthorized | Verify API key is correct |
| 429 | Too Many Requests | Reduce request frequency |
| 500 | Server Error | Retry with exponential backoff |
| 503 | Service Unavailable | Server maintenance, try again later |

### Retry Strategy

For failed requests, implement exponential backoff:

```
1st retry: Wait 1 second
2nd retry: Wait 2 seconds
3rd retry: Wait 4 seconds
4th retry: Wait 8 seconds
5th retry: Wait 16 seconds
```

Maximum retries: 5 attempts

## Best Practices

### 1. Update Frequency

**Recommended intervals**:
- Mobile bins: Every 5-15 minutes
- Stationary bins: Every 30-60 minutes
- Critical monitoring: Every 1-5 minutes

### 2. Data Validation

Always validate GPS data before sending:
- Latitude must be between -90 and 90
- Longitude must be between -180 and 180
- Ensure GPS fix is valid (HDOP < 5.0 recommended)
- Minimum 4 satellites for reliable fix

### 3. Connection Management

- Implement connection timeout (10-30 seconds)
- Handle WiFi disconnections gracefully
- Cache failed updates and retry when connection restored

### 4. Power Management

For battery-powered devices:
- Use deep sleep between updates
- Wake up only to send data
- Consider solar charging for outdoor deployment

### 5. Security

- Store API key securely (don't hardcode in public code)
- Use HTTPS in production
- Validate SSL certificates
- Monitor for unauthorized access attempts

## Testing

### Test with cURL

```bash
curl -X POST https://your-domain.com/api/iot/update-location \
  -H "Content-Type: application/json" \
  -d '{
    "apiKey": "BIN_XXXXXXXXX_XXXXXX",
    "latitude": 8.476876,
    "longitude": 123.799913
  }'
```

### Test with Postman

1. Create new POST request
2. URL: `https://your-domain.com/api/iot/update-location`
3. Headers: `Content-Type: application/json`
4. Body (raw JSON):
   ```json
   {
     "apiKey": "BIN_XXXXXXXXX_XXXXXX",
     "latitude": 8.476876,
     "longitude": 123.799913
   }
   ```
5. Send request

### Test Response Times

Expected response times:
- Local network: 50-200ms
- Internet (good connection): 200-500ms
- Internet (poor connection): 500-2000ms

If response time exceeds 3000ms, consider:
- Checking network connection
- Verifying server performance
- Reducing data payload

## Example Implementations

### Arduino (ESP8266/ESP32)

See `ecoearn_bin_tracker.ino` for complete implementation.

### Python

```python
import requests
import json

def update_location(api_key, latitude, longitude):
    url = "https://your-domain.com/api/iot/update-location"
    
    payload = {
        "apiKey": api_key,
        "latitude": latitude,
        "longitude": longitude
    }
    
    headers = {
        "Content-Type": "application/json"
    }
    
    try:
        response = requests.post(url, json=payload, headers=headers, timeout=10)
        return response.json()
    except Exception as e:
        print(f"Error: {e}")
        return None

# Usage
result = update_location("BIN_XXXXXXXXX_XXXXXX", 8.476876, 123.799913)
print(result)
```

### Node.js

```javascript
const axios = require('axios');

async function updateLocation(apiKey, latitude, longitude) {
  try {
    const response = await axios.post(
      'https://your-domain.com/api/iot/update-location',
      {
        apiKey: apiKey,
        latitude: latitude,
        longitude: longitude
      },
      {
        headers: {
          'Content-Type': 'application/json'
        },
        timeout: 10000
      }
    );
    
    return response.data;
  } catch (error) {
    console.error('Error:', error.message);
    return null;
  }
}

// Usage
updateLocation('BIN_XXXXXXXXX_XXXXXX', 8.476876, 123.799913)
  .then(result => console.log(result));
```

## Monitoring and Logging

### Server-Side Logs

The API logs the following information:
- Timestamp of each request
- API key used (masked for security)
- GPS coordinates received
- Update success/failure status
- Response time

### Client-Side Logs

Your IoT device should log:
- GPS fix acquisition time
- Number of satellites
- HDOP value
- Request success/failure
- Network errors
- Battery level (if applicable)

## Future Features

Planned enhancements:
- Batch location updates
- Real-time bin status monitoring
- Fill level reporting
- Temperature sensors
- Weight sensors
- Authentication refresh tokens
- WebSocket support for real-time updates

## Support

For technical support or questions about the IoT API:
- Email: support@ecoearn.com
- Documentation: https://docs.ecoearn.com
- GitHub Issues: https://github.com/your-repo/issues

## Changelog

### Version 1.0.0 (2025-11-06)
- Initial release
- POST endpoint for location updates
- API key authentication
- Basic error handling and validation
