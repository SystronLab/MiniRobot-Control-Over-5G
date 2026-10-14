from setuptools import setup

package_name = 'demo1_5g_robot_controller'

setup(
    name=package_name,
    version='2.0.1',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='University of York',
    maintainer_email='anthony.moulds@york.ac.uk',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'robot_controller = demo1_5g_robot_controller.robot_controller:main',
        ],
    },
)
