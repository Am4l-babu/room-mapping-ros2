from setuptools import setup

package_name = 'my_robot_description'

setup(
    name=package_name,
    version='0.0.1',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch',
            ['launch/teleop_robot.launch.py',
             'launch/teleop_keyboard_only.launch.py']),
        ('share/' + package_name + '/urdf',
            ['urdf/my_robot.urdf']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    author='ros',
    author_email='ros@example.com',
    maintainer='ros',
    maintainer_email='ros@example.com',
    keywords=['ROS2'],
    classifiers=[
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python',
        'Topic :: Software Development',
    ],
    description='Robot description package including URDF and launch files',
    long_description='',
    license='Apache License 2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'simple_teleop = my_robot_description.simple_teleop:main',
        ],
    },
)
