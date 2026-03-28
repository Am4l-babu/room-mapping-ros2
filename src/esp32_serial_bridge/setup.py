from setuptools import find_packages, setup
from glob import glob
import os

package_name = 'esp32_serial_bridge'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob(os.path.join('launch', '*launch.[pxy][yma]*'))),
        (os.path.join('share', package_name, 'config'), glob(os.path.join('config', '*'))),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ros',
    maintainer_email='amalbabu34767@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'bridge_node = esp32_serial_bridge.bridge_node:main',
            'robot_controller = esp32_serial_bridge.robot_controller:main',
            'micro_ros_robot_bridge = esp32_serial_bridge.micro_ros_robot_bridge:main',
        ],
    },
)
