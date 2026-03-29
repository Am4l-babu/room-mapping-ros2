#!/bin/bash
# setup_config.sh - Initialize configuration files from templates
# 
# This script helps set up configuration files for the WiFi Robot Car project.
# It copies templates to actual config files and provides instructions.

set -e

PROJECT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$PROJECT_ROOT"

echo "============================================================================"
echo "WiFi Robot Car - Configuration Setup"
echo "============================================================================"
echo ""

# Function to setup a config file
setup_config() {
    local TEMPLATE=$1
    local CONFIG=$2
    local DESCRIPTION=$3
    
    echo "📋 Setting up $DESCRIPTION..."
    
    if [ -f "$CONFIG" ]; then
        echo "   ⚠️  $CONFIG already exists. Skipping."
        return 0
    fi
    
    if [ ! -f "$TEMPLATE" ]; then
        echo "   ❌ $TEMPLATE not found!"
        return 1
    fi
    
    cp "$TEMPLATE" "$CONFIG"
    echo "   ✅ Created $CONFIG from template"
    echo "   📝 Please edit $CONFIG and replace CHANGE_ME_* values"
    echo ""
}

# ============================================================================
# Setup C++ Configuration
# ============================================================================
echo "🔧 C++ Configuration"
echo "───────────────────────────────────────────────────────────────────────────"
setup_config \
    "sensor_testing/include/config_template.h" \
    "sensor_testing/include/config.h" \
    "C++ Configuration (esp32/sensor_testing)"

# ============================================================================
# Setup Python Configuration  
# ============================================================================
echo "🐍 Python Configuration"
echo "───────────────────────────────────────────────────────────────────────────"
setup_config \
    "robot_web_control/config_template.py" \
    "robot_web_control/config.py" \
    "Python Configuration (Robot Web Control)"

# ============================================================================
# Verify Git Configuration
# ============================================================================
echo "🔒 Git Configuration"
echo "───────────────────────────────────────────────────────────────────────────"

if grep -q "sensor_testing/include/config.h" .gitignore 2>/dev/null && \
   grep -q "robot_web_control/config.py" .gitignore 2>/dev/null; then
    echo "   ✅ .gitignore properly configured"
    echo "   📋 The following files are excluded from git:"
    echo "      - sensor_testing/include/config.h (C++)"
    echo "      - robot_web_control/config.py (Python)"
else
    echo "   ⚠️  .gitignore may not be fully configured"
fi
echo ""

# ============================================================================
# Summary and Next Steps
# ============================================================================
echo "✨ Configuration Setup Complete!"
echo "───────────────────────────────────────────────────────────────────────────"
echo ""
echo "📝 NEXT STEPS:"
echo ""
echo "1. Edit your configuration files and replace CHANGE_ME_* values:"
echo "   ${PROJECT_ROOT}/sensor_testing/include/config.h"
echo "   ${PROJECT_ROOT}/robot_web_control/config.py"
echo ""
echo "2. Verify your WiFi credentials are correct"
echo ""
echo "3. Build and test:"
echo "   cd sensor_testing && pio run -e wifi_car"
echo ""
echo "4. Start the web interface:"
echo "   cd robot_web_control && python3 app.py"
echo ""
echo "⚠️  SECURITY REMINDER:"
echo "   - NEVER commit config.h or config.py to git!"
echo "   - Only share config_template.* with your team"
echo "   - Use strong, unique passwords"
echo "   - Keep SSH keys and API tokens secure"
echo ""
echo "📚 Documentation:"
echo "   See TEMPLATE_SETUP_GUIDE.md for detailed information"
echo ""
