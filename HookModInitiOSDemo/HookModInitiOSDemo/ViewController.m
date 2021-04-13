//
//  ViewController.m
//  HookModInitiOSDemo
//
//  Created by kyson on 2021/4/10.
//

#import "ViewController.h"

@interface ViewController ()

@end

@implementation ViewController


const char* getallinitinfo();



- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    //dispatch_time_t参数
    dispatch_time_t time = dispatch_time(DISPATCH_TIME_NOW, (ino64_t)(4 * NSEC_PER_SEC));

    //dispatch_queue_t参数
    dispatch_queue_t queue = dispatch_get_main_queue();//主队列

    //dispatch_after函数
    dispatch_after(time, queue, ^{
        //Code
        NSLog(@"allinitinfo = %s",getallinitinfo());

    });

}


@end
